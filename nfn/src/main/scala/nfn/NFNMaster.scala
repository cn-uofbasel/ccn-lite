package nfn

import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.util.{Success, Failure}

import java.io.{PrintWriter, StringWriter}

import akka.actor._
import akka.util.ByteString
import akka.event.Logging

import network._
import network.UDPConnection._
import nfn.service._
import nfn.NFNMaster._
import ccn.ccnlite.CCNLite
import ccn.packet._
import ccn.{ContentStore, CCNLiteProcess}
import java.net.InetSocketAddress
import myutil.IOHelper
import lambdacalculus.parser.ast.{LambdaNFNPrinter, LambdaLocalPrettyPrinter, Variable, Expr}
import nfn.local.LocalAbstractMachineWorker
import monitor.MonitorActor
import monitor.MonitorActor.{ContentLog, InterestLog, NodeLog}


object NFNMaster {

  def byteStringToPacket(byteArr: Array[Byte]): Option[Packet] = {
    NFNCommunication.parseCCNPacket(CCNLite.ccnbToXml(byteArr))
  }

  case class CCNSendReceive(interest: Interest) {
    def this(expr: Expr, local: Boolean = false) = {
      this(Interest(
        expr match {
          case Variable(name, _) => Seq(name)
          case _ => Seq(
            if(local) LambdaLocalPrettyPrinter(expr)
            else LambdaNFNPrinter(expr),
            "NFN"
          )
        }
      ))
    }
  }

  case class CCNAddToCache(content: Content)

  case class ComputeResult(content: Content)

  case class Connect(nodeConfig: NodeConfig)

  case class Thunk(interest: Interest)

  case class Exit()

}

case class NodeConfig(host: String, port: Int, computePort: Int, prefix: String) {
  def toNFNNodeLog: NodeLog = NodeLog(host, port, Some(prefix), Some("NFNNode"))
  def toComputeNodeLog: NodeLog = NodeLog(host, computePort, Some(prefix + "compute"), Some("ComputeNode"))
}

object NFNMasterFactory {
  def network(context: ActorRefFactory, nodeConfig: NodeConfig) = {
    context.actorOf(networkProps(nodeConfig), name = "NFNMasterNetwork")
  }

  def networkProps(nodeConfig: NodeConfig) = Props(classOf[NFNMasterNetwork], nodeConfig)

  def local(context: ActorRefFactory) = {
    context.actorOf(localProps, name = "NFNMasterLocal")
  }
  def localProps: Props = Props(classOf[NFNMasterLocal])
}


/**
 * Master Actor which is basically a simple ccn router with a content store and a pednign interest table but with only one forwarding rule
 * (an actual ccn router). It forwards compute requests to a [[ComputeWorker]].
 */
trait NFNMaster extends Actor {

  val logger = Logging(context.system, this)
  val name = self.path.name

  val ccnIf = CCNLite

  val cs = ContentStore()
  var pit: Map[Seq[String], Set[ActorRef]] = Map()


  private def createComputeWorker(interest: Interest): ActorRef =
    context.actorOf(Props(classOf[ComputeWorker], self), s"ComputeWorker-${interest.hashCode}")

  private def handleInterest(interest: Interest) = {
    cs.find(interest.name) match {
      case Some(content) => sender ! content
      case None => {
        pit.get(interest.name) match {
          case Some(_) => {
            val pendingFaces = pit(interest.name) + sender
            pit += (interest.name -> pendingFaces )
          }
          case None =>
            val computeWorker = createComputeWorker(interest)
            val updatedSendersForInterest = pit.get(interest.name).getOrElse(Set()) + computeWorker
            pit += (interest.name -> updatedSendersForInterest)
            computeWorker.tell(interest, self)
        }
      }
    }
  }


  // Check pit for an address to return content to, otherwise discard it
  private def handleContent(content: Content) = {
    if(pit.get(content.name).isDefined) {
      pit(content.name) foreach { pendingSender =>  pendingSender ! content}
      pit -= content.name
    } else {
      logger.error(s"Discarding content $content because there is no entry in pit")
    }
  }


  def handlePacket(packet: CCNPacket) = {
    logger.info(s"Received: $packet")
    monitorReceive(packet)
    packet match {
      case i: Interest => handleInterest(i)
      case c: Content => handleContent(c)
    }
  }

  def monitorReceive(packet: CCNPacket)


  override def receive: Actor.Receive = {

    // received Data from network
    // If it is an interest, start a compute request
//    case CCNReceive(packet) => handlePacket(packet)
    case packet:CCNPacket => handlePacket(packet)


    case UDPConnection.Received(data, sendingRemote) => {
      val maybePacket = byteStringToPacket(data)
      logger.debug(s"$name received ${maybePacket.getOrElse("unparsable data")}")
      maybePacket match {
        // Received an interest from the network (byte format) -> spawn a new worker which handles the messages (if it crashes we just assume a timeout at the moment)
        case Some(packet: CCNPacket) => handlePacket(packet)
        case Some(AddToCache()) => ???
        case None => logger.warning(s"Received data which cannot be parsed to a ccn packet: ${new String(data)}")
      }
    }

    case CCNSendReceive(interest) => {
      cs.find(interest.name) match {
        case Some(content) => {
          logger.info(s"Received SendReceive request, found content for interest $interest in local CS")
          sender ! content
        }
        case None => {
          logger.info(s"Received SendReceive request, aksing network for $interest ")
          val updatedSendersForInterest = pit.get(interest.name).getOrElse(Set())  + sender
          pit += (interest.name -> updatedSendersForInterest)
          send(interest)
        }
      }
    }

    case Thunk(interest) => {
      logger.debug(s"sending thunk for interest $interest")
      send(Content(interest.name, "THUNK".getBytes))
    }

    // CCN worker itself is responsible to handle compute requests from the network (interests which arrived in binary format)
    // convert the result to ccnb format and send it to the socket
    case ComputeResult(content) => {
      pit.get(content.name) match {
        case Some(workers) => {
          logger.debug("sending compute result to socket")
          send(content)
          sender ! ComputeWorker.Exit()
          pit -= content.name
        }
        case None => logger.error(s"Received result from compute worker which timed out, discarding the result content: $content")
      }
    }

    case CCNAddToCache(content) => {
      logger.info(s"sending add to cache for name ${content.name.mkString("/", "/", "")}")
      sendAddToCache(content)
    }

    // TODO this message is only for network node
    case Connect(otherNodeConfig) => {
      connect(otherNodeConfig)
    }

    case Exit() => {
      exit()
      context.system.shutdown()
    }

  }

  def connect(otherNodeConfig: NodeConfig): Unit
  def send(packet: CCNPacket): Unit
  def sendAddToCache(content: Content): Unit
  def exit(): Unit = ()
}

case class NFNMasterNetwork(nodeConfig: NodeConfig) extends NFNMaster {

  val ccnLiteNFNNetworkProcess = CCNLiteProcess(nodeConfig)

  MonitorActor.monitor ! MonitorActor.ConnectLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog)
  MonitorActor.monitor ! MonitorActor.ConnectLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog)

  ccnLiteNFNNetworkProcess.start()

  val nfnSocket = context.actorOf(Props(classOf[UDPConnection],
                                          new InetSocketAddress(nodeConfig.host, nodeConfig.computePort),
                                          Some(new InetSocketAddress(nodeConfig.host, nodeConfig.port))),
                                        name = s"udpsocket-${nodeConfig.computePort}-${nodeConfig.port}")



  override def preStart() = {
    nfnSocket ! Handler(self)
  }


  override def send(packet: CCNPacket): Unit = {
    nfnSocket ! Send(ccnIf.mkBinaryPacket(packet))

    val ccnb = NFNCommunication.encodeBase64(ccnIf.mkBinaryPacket(packet))
    MonitorActor.monitor ! MonitorActor.PacketLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog, true, InterestLog(packet.toString))
  }

  override def sendAddToCache(content: Content): Unit = {
    nfnSocket ! Send(ccnIf.mkAddToCacheInterest(content))
  }

  override def exit(): Unit = {
    ccnLiteNFNNetworkProcess.stop()
  }

  override def connect(otherNodeConfig: NodeConfig): Unit = {
    ccnLiteNFNNetworkProcess.connect(otherNodeConfig)
    MonitorActor.monitor ! MonitorActor.ConnectLog(nodeConfig.toNFNNodeLog, otherNodeConfig.toNFNNodeLog)
  }

  override def monitorReceive(packet: CCNPacket): Unit = {
    packet match {
      case i: Interest => MonitorActor.monitor ! MonitorActor.PacketLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog, isSent = false, InterestLog(i.name.mkString("/", "/", "")))
      case c: Content => {
        val name = c.name.mkString("/", "/", "")
        val data = new String(c.data).take(50)
        MonitorActor.monitor ! MonitorActor.PacketLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog, isSent = false, ContentLog(name, data))
      }
    }
  }
}

case class NFNMasterLocal() extends NFNMaster {

  val localAM = context.actorOf(Props(classOf[LocalAbstractMachineWorker], self), name = "localAM")

  override def send(packet: CCNPacket): Unit = localAM ! packet

  override def sendAddToCache(content: Content): Unit = {
    cs.add(content)
  }

  override def connect(otherNodeConfig: NodeConfig): Unit = ???

  override def monitorReceive(packet: CCNPacket): Unit = ???
}





