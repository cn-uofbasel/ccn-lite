package nfn

import scala.concurrent.ExecutionContext.Implicits.global

import java.net.InetSocketAddress

import akka.actor._
import akka.event.Logging

import network._
import nfn.NFNServer._
import ccn.ccnlite.CCNLite
import ccn.packet._
import ccn._
import lambdacalculus.parser.ast._
import monitor.Monitor
import monitor.Monitor.PacketLogWithoutConfigs


object NFNServer {

  def byteStringToPacket(byteArr: Array[Byte]): Option[Packet] = {
    NFNCommunication.parseCCNPacket(CCNLite.ccnbToXml(byteArr))
  }

  case class ComputeResult(content: Content)

  case class Thunk(interest: Interest, executionTimeEstimated: Option[Int])

  case class Exit()

}

object NFNApi {

  object CCNSendReceive {
    def fromExpression(expr: Expr, local: Boolean = false): CCNSendReceive = {
      val nameCmps: Seq[String] =
        expr match {
          case Variable(name, _) => Seq(name)
          case _ =>
            if (local) Seq(LambdaPrettyPrinter(expr))
            else Seq(LambdaNFNPrinter(expr), "NFN")
        }
      CCNSendReceive(Interest(nameCmps: _*))
    }
  }

  case class CCNSendReceive(interest: Interest, useThunks: Boolean = false)

  case class AddToCCNCache(content: Content)

  case class AddToLocalCache(content: Content, prependLocalPrefix: Boolean = false)

  case class Connect(nodeConfig: NodeConfig)

  case class AddCCNFace(nodeConfig: NodeConfig, gateway: NodeConfig)

  case class AddLocalFace(nameWithoutPrefix: String)
}



object CCNServerFactory {
  def ccnServer(context: ActorRefFactory, nodeConfig: NodeConfig) = {
    context.actorOf(networkProps(nodeConfig), name = "NFNServer")
  }
  def networkProps(nodeConfig: NodeConfig) = Props(classOf[NFNServer], nodeConfig)
}





class UDPConnectionContentInterest(local:InetSocketAddress,
                                   target:InetSocketAddress) extends UDPConnection(local, Some(target)) {

  def logPacket(packet: CCNPacket) = {
    val ccnPacketLog = packet match {
      case i: Interest => Monitor.InterestInfoLog("interest", i.name.toString)
      case c: Content => Monitor.ContentInfoLog("content", c.name.toString, c.possiblyShortenedDataString)
    }
    Monitor.monitor ! new PacketLogWithoutConfigs(local.getHostString, local.getPort, target.getHostString, target.getPort, true, ccnPacketLog)
  }

  def handlePacket(packet: CCNPacket) =
    packet match {
      case i: Interest =>
        val binaryInterest = CCNLite.mkBinaryInterest(i)
        self.tell(UDPConnection.Send(binaryInterest), sender)
      case c: Content =>
        val binaryContent = CCNLite.mkBinaryContent(c)
        self.tell(UDPConnection.Send(binaryContent), sender)
    }

  def interestContentReceiveWithoutLog: Receive = {
    case p: CCNPacket => handlePacket(p)
  }

  def interestContentReceive: Receive = {
    case p: CCNPacket => {
      logPacket(p)
      handlePacket(p)
    }
  }

  override def receive = super.receive orElse interestContentReceiveWithoutLog

  override def ready(actorRef: ActorRef) = super.ready(actorRef) orElse interestContentReceive
}

/**
 * The NFNServer is the gateway interface to the CCNNetwork and provides the NFNServer implements the [[NFNApi]].
 * It manages a local cs form where any incoming content requests are served.
 * It also maintains a pit for received interests. Since everything is akka based, the faces in the pit are [[ActorRef]],
 * this means that usually these refs are to:
 *  - interest from the network with the help of [[UDPConnection]] (or any future connection type)
 *  - the [[ComputeServer]] or [[ComputeWorker]]can make use of the pit
 *  - any interest with the help of the akka "ask" pattern
 *  All connection, interest and content request are logged to the [[Monitor]].
 *  A NFNServer also maintains a socket which is connected to the actual CCNNetwork, usually an CCNLite instance encapsulated in a [[CCNLiteProcess]].
 */
case class NFNServer(nodeConfig: NodeConfig) extends Actor {

  val logger = Logging(context.system, this)

  val ccnIf = CCNLite

  val cs = ContentStore()
  var pit: Map[CCNName, Set[ActorRef]] = Map()

  val cacheContent: Boolean = true

  var maybeLocalAbstractMachine: Option[ActorRef] = None


  Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog)
  Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog)

  val ccnLiteNFNNetworkProcess = CCNLiteProcess(nodeConfig)

  ccnLiteNFNNetworkProcess.start()

  val computeServer: ActorRef = context.actorOf(Props(classOf[ComputeServer]), name = "ComputeServer")

  val nfnGateway = context.actorOf(Props(classOf[UDPConnectionContentInterest],
    new InetSocketAddress(nodeConfig.host, nodeConfig.computePort),
    new InetSocketAddress(nodeConfig.host, nodeConfig.port)),
    name = s"udpsocket-${nodeConfig.computePort}-${nodeConfig.port}")



  override def preStart() = {
    nfnGateway ! UDPConnection.Handler(self)
  }



  // Check pit for an address to return content to, otherwise discard it
  private def handleContent(content: Content) = {

    def handleThunkContent = {
      pit.get(content.name) match {
        case Some(pendingFaces) => {
          val (contentNameWithoutThunk, isThunk) = content.name.withoutThunkAndIsThunk

          assert(isThunk, s"handleThunkContent received the content object $content which is not a thunk")

          // if the thunk content name is a compute name it means that the content was sent by the compute server
          // send the thunk content to the gateway
          if(content.name.isCompute) {
            nfnGateway ! content

          // else it was a thunk interest, which means we can now send the actual interest
          } else {
            val interest = Interest(contentNameWithoutThunk)
            logger.debug(s"Received thunk $content, sending actual interest $interest")
            nfnGateway ! interest
            pit += contentNameWithoutThunk -> pendingFaces
          }
          pit -= content.name
        }
        case None => logger.error(s"Discarding thunk content $content because there is no entry in pit")
      }
    }
    def handleNonThunkContent = {

      pit.get(content.name) match {
        case Some(pendingFaces) => {
          if (content.name.isCompute) {
            logger.debug("Received computation result, sending back computation ack")
            computeServer ! ComputeServer.ComputationFinished(content.name)
          }

          if (cacheContent) {
            cs.add(content)
          }

          pendingFaces foreach { pendingSender => pendingSender ! content }
          pit -= content.name
        }
        case None =>
          logger.warning(s"Discarding content $content because there is no entry in pit")
      }
    }

    if(content.name.isThunk) {
      handleThunkContent
    } else {
      handleNonThunkContent
    }
  }

  private def handleInterest(i: Interest) = {
    cs.find(i.name) match {
      case Some(contentFromLocalCS) =>
        sender ! contentFromLocalCS

      case None => {

        pit.get(i.name) match {
          case Some(pendingFaces) =>
            pit += i.name -> (pendingFaces + sender)
          case None => {
            pit += i.name -> Set(sender)

            // /.../.../NFN
            if (i.name.isNFN) {
              val useThunks = i.name.isThunk
              // /COMPUTE/call .../.../NFN
              // A compute flag at the beginning means that the interest is a binary computation
              // to be executed on the compute server
              if (i.name.isCompute) {
                // TODO forward to compute server wit
                // TODO sue thunks for computeserver
                computeServer ! ComputeServer.Compute(i.name, useThunks)

                // /.../.../NFN
                // An NFN interest without compute flag means that it must be reduced by an abstract machine
                // If no local machine is avaialbe, forward it to the nfn network
              } else {
                maybeLocalAbstractMachine match {
                  case Some(localAbstractMachine) => {
                    // TODO send to local AM
                    localAbstractMachine ! i
                  }
                  case None => nfnGateway ! i
                }
              }
            } else {
              nfnGateway ! i
            }
          }
        }
      }
    }
  }


  def addToPit(name: CCNName, interestSender: ActorRef) = {
    pit += name -> (pit.get(name).getOrElse(Set[ActorRef]()) + interestSender)
  }

  def handlePacket(packet: CCNPacket) = {
    monitorReceive(packet)
    val isThunk = packet.name.isThunk
    packet match {
      case i: Interest => {
        logger.info(s"Received interest: $i")
        handleInterest(i)
      }
      case c: Content => {
        logger.info(s"Received content: $c")
        handleContent(c)
      }
    }
  }

//  def sendThunkContentForName(name: CCNName, executionTimeEstimated: Option[Int]): Unit = {
//    val thunkContent = Content.thunkForName(name, executionTimeEstimated)
//    logger.debug(s"sending thunk answer ${thunkContent}")
//    sendToNetwork(thunkContent)
//  }


  override def receive: Actor.Receive = {
    // received Data from network
    // If it is an interest, start a compute request
    case packet:CCNPacket => handlePacket(packet)
    case UDPConnection.Received(data, sendingRemote) => {
      val maybePacket = byteStringToPacket(data)
      maybePacket match {
        // Received an interest from the network (byte format) -> spawn a new worker which handles the messages (if it crashes we just assume a timeout at the moment)
        case Some(packet: CCNPacket) => handlePacket(packet)
        case Some(AddToCache()) => ???
        case None => logger.debug(s"Received data which cannot be parsed to a ccn packet: ${new String(data)}")
      }
    }

    /**
     * [[NFNApi.CCNSendReceive]] is a message of the external API which retrieves the content for the interest and sends it back to the sender actor.
     * The sender actor can be initialized from an ask patter or form another actor.
     * It tries to first serve the interest from the local cs, otherwise it adds an entry to the pit
     * and asks the network if it was the first entry in the pit.
     * Thunk interests get converted to normal interests, thunks need to be enabled with the boolean flag in the message
     */
    case NFNApi.CCNSendReceive(maybeThunkInterest, useThunks) => {
      val interest =
        if(useThunks) maybeThunkInterest.thunkify
        else          maybeThunkInterest
      handlePacket(interest)
    }

//    case Thunk(interest, executionTimeEstimated) => {
//      sendThunkContentForName(interest.name, executionTimeEstimated)
//    }

    case NFNApi.AddToCCNCache(content) => {
      logger.info(s"sending add to cache for name ${content.name}")
      sendAddToCache(content)
    }

    case NFNApi.AddToLocalCache(content, prependLocalPrefix) => {
      val contentToAdd =
      if(prependLocalPrefix) {
        Content(nodeConfig.prefix.append(content.name), content.data)
      } else content
      logger.info(s"Adding content for ${contentToAdd.name} to local cache")
      cs.add(contentToAdd)
    }

    // TODO this message is only for network node
    case NFNApi.Connect(otherNodeConfig) => {
      connect(otherNodeConfig)
    }

    // TODO this message is only for network node
    case NFNApi.AddCCNFace(otherNodeConfig, gateway) => {
      addPrefix(otherNodeConfig, gateway)
    }


    case Exit() => {
      exit()
      context.system.shutdown()
    }

  }

//  def sendToNetwork(packet: CCNPacket): Unit = {
//
//
//    nfnGateway ! UDPConnection.Send(ccnIf.mkBinaryPacket(packet))
//  }

  def sendAddToCache(content: Content): Unit = {
    nfnGateway ! UDPConnection.Send(ccnIf.mkAddToCacheInterest(content))
  }

  def exit(): Unit = {
    computeServer ! PoisonPill
    nfnGateway ! PoisonPill
    ccnLiteNFNNetworkProcess.stop()
  }

  def connect(otherNodeConfig: NodeConfig): Unit = {
    Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toNFNNodeLog, otherNodeConfig.toNFNNodeLog)
    ccnLiteNFNNetworkProcess.connect(otherNodeConfig)
  }

  def addPrefix(prefixNode: NodeConfig, gatewayNode: NodeConfig): Unit = {
    // TODO: faces are not logged / visualized in any way
    ccnLiteNFNNetworkProcess.addPrefix(prefixNode, gatewayNode)
  }

  def monitorReceive(packet: CCNPacket): Unit = {
    val ccnPacketLog = packet match {
      case i: Interest => Monitor.InterestInfoLog("interest", i.name.toString)
      case c: Content => {
        val name = c.name.toString
        val data = new String(c.data).take(50)
        Monitor.ContentInfoLog("content", name, data)
      }
    }
    Monitor.monitor ! new Monitor.PacketLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog, isSent = false, ccnPacketLog)
  }
}
