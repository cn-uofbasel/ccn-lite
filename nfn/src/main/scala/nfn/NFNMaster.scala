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
import monitor.Monitor
import monitor.Monitor.{ContentInfoLog, InterestInfoLog, NodeLog}


object NFNMaster {

  def byteStringToPacket(byteArr: Array[Byte]): Option[Packet] = {
    NFNCommunication.parseCCNPacket(CCNLite.ccnbToXml(byteArr))
  }

  object CCNSendReceive {
    def fromExpression(expr: Expr, local: Boolean = false): CCNSendReceive = {
      val nameCmps: Seq[String] =
        expr match {
          case Variable(name, _) => Seq(name)
          case _ =>
            if (local) Seq(LambdaLocalPrettyPrinter(expr))
            else Seq(LambdaNFNPrinter(expr), "NFN")
        }
      CCNSendReceive(Interest(nameCmps: _*))
    }
  }

  case class CCNSendReceive(interest: Interest, useThunks: Boolean = false)

  case class CCNAddToCache(content: Content)

  case class ComputeResult(content: Content)

  case class Connect(nodeConfig: NodeConfig)

  case class Thunk(interest: Interest)

  case class Exit()

}

case class NodeConfig(host: String, port: Int, computePort: Int, prefix: String) {
  def toNFNNodeLog: NodeLog = NodeLog(host, port, Some("NFNNode"), Some(prefix))
  def toComputeNodeLog: NodeLog = NodeLog(host, computePort, Some("ComputeNode"), Some(prefix + "compute"))
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
  logger.debug("init")

  val ccnIf = CCNLite

  val cs = ContentStore()
  var pit: Map[CCNName, Set[ActorRef]] = Map()


  private def createComputeWorker(interest: Interest): ActorRef =
    context.actorOf(Props(classOf[ComputeWorker], self), s"ComputeWorker-${interest.name.hashCode}")

  /*
   * Handles an interests to the compute server with the following flow:
   * First, checks if the interest is a thunk or not.
   * It looks up the name of the interest as a normal interest without the thunk component in the content store.
   * If it founds the content, it either sends back the thunk answer or the content itself.
   * If it does not find the content, it does not differentiate between thunk or not anymore.
   * The interest is simply added to the pit if the interest is already pending or
   * a new compute worker is initialized and started and an entry is added to the pit.
   * If the interest is a thunk, the computeWorker handles the interest the following:
   * it first tries to receive all services and data objects,
   * if it has received all it sends thunk content back before starting to execute the result.
   */
  private def handleInterestToComputeServer(interest: Interest) = {

    val (nameWithoutThunk, isThunk) = interest.name.withoutThunkAndIsThunk

      cs.find(nameWithoutThunk) match {
        case Some(content) => {
          if (isThunk) {
            sendThunkContentForName(nameWithoutThunk)
          } else {
            logger.debug(s"Found content $content in content store, send it back")
            send(content)
          }
        }
        case None => {
          pit.get(interest.name) match {
            case Some(pendingFaces) => {
              logger.debug(s"Received interest which is already in pit, add it to the pit")
              val pendingFacesWithSender = pendingFaces + sender
              pit += (nameWithoutThunk -> pendingFacesWithSender)
            }
            case None =>
              logger.debug(s"Received interest which is not in pit, create compute worker and forward to it")
              val computeWorker = createComputeWorker(interest)
              val pendingFaces = Set(computeWorker)
              pit += (interest.name -> pendingFaces)
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

  private def handleThunkContent(thunkContent: Content) = {
    pit.get(thunkContent.name) match {
      case Some(_) => {
        val (contentNameWithoutThunk, isThunk) = thunkContent.name.withoutThunkAndIsThunk

        assert(isThunk, s"handleThunkContent received the content object $thunkContent which is not a thunk")

        send(Interest(contentNameWithoutThunk))
        pit -= thunkContent.name
      }
      case None => logger.error(s"Discarding thunk content $thunkContent because there is no entry in pit")
    }

  }

  def handlePacket(packet: CCNPacket) = {
    logger.info(s"Received: $packet")
    monitorReceive(packet)
    packet match {
      case i: Interest => handleInterestToComputeServer(i)
      case c: Content => {
        if(new String(c.data) == "THUNK") {
          handleThunkContent(c)
        } else {
          handleContent(c)
        }
      }
    }
  }

  def monitorReceive(packet: CCNPacket)


  def sendThunkContentForName(name: CCNName): Unit = {
    val thunkContent = Content.thunkForName(name)
    logger.debug(s"sending thunk ${thunkContent.name}")
    send(thunkContent)
  }

  override def receive: Actor.Receive = {

    // received Data from network
    // If it is an interest, start a compute request
//    case CCNReceive(packet) => handlePacket(packet)
    case packet:CCNPacket => handlePacket(packet)


    case UDPConnection.Received(data, sendingRemote) => {
      val maybePacket = byteStringToPacket(data)
      logger.debug(s"Received ${maybePacket.getOrElse("unparsable data")}")
      maybePacket match {
        // Received an interest from the network (byte format) -> spawn a new worker which handles the messages (if it crashes we just assume a timeout at the moment)
        case Some(packet: CCNPacket) => handlePacket(packet)
        case Some(AddToCache()) => ???
        case None => logger.debug(s"Received data which cannot be parsed to a ccn packet: ${new String(data)}")
      }
    }

    /**
     * [[CCNSendReceive]] is a message of the external API which retrieves the content for the interest and sends it back to the sender actor.
     * The sender actor can be initialized from an ask patter or form another actor.
     * It tries to first serve the interest from the local cs, otherwise it adds an entry to the pit
     * and asks the network if it was the first entry in the pit.
     * Thunk interests get converted to normal interests, thunks need to be enabled with the boolean flag in the message
     */
    case CCNSendReceive(maybeThunkInterest, useThunks) => {
      val (nameWithoutThunk, isThunk) = maybeThunkInterest.name.withoutThunkAndIsThunk

      if(isThunk) {
        logger.warning("A thunk message was sent to CCNSentReceive, treating it as an normal interest without thunks. Use the flag in the CCNSentReceive message to enable or disable thunks")
      }

//      val interest =
//        if(useThunks) {
//          maybeThunkInterest.thunkify
//        } else {
//          Interest(nameWithoutThunk)
//        }

      cs.find(nameWithoutThunk) match {
        case Some(content) => {
          logger.info(s"Received SendReceive request, found content for interest $content in local CS")
          sender ! content
        }
        case None => {
          pit.get(nameWithoutThunk) match {
            case Some(pendingFaces) =>  {
              logger.info(s"Received SendReceive request $nameWithoutThunk, entry already exists in pit, just add sender to pending faces")
              pit += (nameWithoutThunk -> (pendingFaces + sender))
            }
            case None => {
              logger.info(s"Received SendReceive request $nameWithoutThunk, no entry in pit, asking network")
              pit += (nameWithoutThunk -> Set(sender))

              if(useThunks) {
                val thunkInterest = Interest(nameWithoutThunk.thunkify)
                pit += thunkInterest.name -> pit.getOrElse(thunkInterest.name, Set()).+(sender)
                send(thunkInterest)
              } else {
                val interest = Interest(nameWithoutThunk)
                send(interest)
              }
            }
          }
        }
      }
    }

    case Thunk(interest) => {
      sendThunkContentForName(interest.name)
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
        case None => logger.warning(s"Received result from compute worker which timed out, discarding the result content: $content")
      }
    }

    case CCNAddToCache(content) => {
      logger.info(s"sending add to cache for name ${content.name}")
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


  Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog)
  Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog)

  val ccnLiteNFNNetworkProcess = CCNLiteProcess(nodeConfig)

  ccnLiteNFNNetworkProcess.start()

  val nfnSocket = context.actorOf(Props(classOf[UDPConnection],
                                          new InetSocketAddress(nodeConfig.host, nodeConfig.computePort),
                                          Some(new InetSocketAddress(nodeConfig.host, nodeConfig.port))),
                                        name = s"udpsocket-${nodeConfig.computePort}-${nodeConfig.port}")



  override def preStart() = {
    nfnSocket ! Handler(self)
  }


  override def send(packet: CCNPacket): Unit = {
    val ccnPacketLog = packet match {
      case i: Interest => InterestInfoLog("interest", i.name.toString)
      case c: Content => ContentInfoLog("content", c.name.toString, c.possiblyShortenedDataString)
    }
    Monitor.monitor ! new Monitor.PacketLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog, true, ccnPacketLog)

    nfnSocket ! Send(ccnIf.mkBinaryPacket(packet))
  }

  override def sendAddToCache(content: Content): Unit = {
    nfnSocket ! Send(ccnIf.mkAddToCacheInterest(content))
  }

  override def exit(): Unit = {
    ccnLiteNFNNetworkProcess.stop()
  }

  override def connect(otherNodeConfig: NodeConfig): Unit = {
    Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toNFNNodeLog, otherNodeConfig.toNFNNodeLog)
    ccnLiteNFNNetworkProcess.connect(otherNodeConfig)
  }

  override def monitorReceive(packet: CCNPacket): Unit = {
    val ccnPacketLog = packet match {
      case i: Interest => InterestInfoLog("interst", i.name.toString)
      case c: Content => {
        val name = c.name.toString
        val data = new String(c.data).take(50)
        ContentInfoLog("content", name, data)
      }
    }
    Monitor.monitor ! new Monitor.PacketLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog, isSent = false, ccnPacketLog)
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





