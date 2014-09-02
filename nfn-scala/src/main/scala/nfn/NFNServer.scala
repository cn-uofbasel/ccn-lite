package nfn

import java.io.{File, FileOutputStream}
import java.net.InetSocketAddress

import akka.actor._
import akka.event.Logging
import akka.pattern._
import akka.util.Timeout
import ccn._
import ccn.ccnlite.CCNLite
import ccn.packet._
import config.AkkaConfig
import monitor.Monitor
import monitor.Monitor.PacketLogWithoutConfigs
import network._
import nfn.NFNServer._
import nfn.localAbstractMachine.LocalAbstractMachineWorker

import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.concurrent.duration._


object NFNServer {

  def byteStringToPacket(byteArr: Array[Byte]): Option[Packet] = {
    NFNCCNLiteParser.parseCCNPacket(CCNLite.ccnbToXml(byteArr))
  }

  case class ComputeResult(content: Content)

  case class Exit()
}

object NFNApi {

  case class CCNSendReceive(interest: Interest, useThunks: Boolean)

  case class AddToCCNCache(content: Content)

  case class AddToLocalCache(content: Content, prependLocalPrefix: Boolean = false)

}


object NFNServerFactory {
  def nfnServer(context: ActorRefFactory, nfnNodeConfig: RouterConfig, computeNodeConfig: ComputeNodeConfig) = {
    context.actorOf(networkProps(nfnNodeConfig, computeNodeConfig), name = "NFNServer")
  }
  def networkProps(nfnNodeConfig: RouterConfig, computeNodeConfig: ComputeNodeConfig) = Props(classOf[NFNServer], nfnNodeConfig, computeNodeConfig)
}


class UDPConnectionContentInterest(local:InetSocketAddress,
                                   target:InetSocketAddress) extends UDPConnection(local, Some(target)) {

  def logPacket(packet: CCNPacket) = {
    val ccnPacketLog = packet match {
      case i: Interest => Monitor.InterestInfoLog("interest", i.name.toString)
      case c: Content => Monitor.ContentInfoLog("content", c.name.toString, c.possiblyShortenedDataString)
      case n: NAck => Monitor.ContentInfoLog("content", n.name.toString, ":NACK")
    }
    Monitor.monitor ! new PacketLogWithoutConfigs(local.getHostString, local.getPort, target.getHostString, target.getPort, true, ccnPacketLog)
  }

  def handlePacket(packet: CCNPacket, senderCopy: ActorRef) =
    packet match {
      case i: Interest =>
        val binaryInterest = CCNLite.mkBinaryInterest(i)
        self.tell(UDPConnection.Send(binaryInterest), senderCopy)
      case c: Content =>
        val binaryContent = CCNLite.mkBinaryContent(c)
        self.tell(UDPConnection.Send(binaryContent), senderCopy)
      case n: NAck =>
        val binaryContent = CCNLite.mkBinaryContent(Content(n.name, n.content.getBytes))
        self.tell(UDPConnection.Send(binaryContent), senderCopy)
    }

  def interestContentReceiveWithoutLog: Receive = {
    case p: CCNPacket => handlePacket(p, sender)
  }

  def interestContentReceive: Receive = {
    case p: CCNPacket => {
      logPacket(p)
      handlePacket(p, sender)
    }
  }

  override def receive = super.receive orElse interestContentReceiveWithoutLog

  override def ready(actorRef: ActorRef) = super.ready(actorRef) orElse interestContentReceive
}

/**
 * The NFNServer is the gateway interface to the CCNNetwork and provides the NFNServer implements the [[NFNApi]].
 * It manages a localAbstractMachine cs form where any incoming content requests are served.
 * It also maintains a pit for received interests. Since everything is akka based, the faces in the pit are [[ActorRef]],
 * this means that usually these refs are to:
 *  - interest from the network with the help of [[UDPConnection]] (or any future connection type)
 *  - the [[ComputeServer]] or [[ComputeWorker]]can make use of the pit
 *  - any interest with the help of the akka "ask" pattern
 *  All connection, interest and content request are logged to the [[Monitor]].
 *  A NFNServer also maintains a socket which is connected to the actual CCNNetwork, usually an CCNLite instance encapsulated in a [[CCNLiteProcess]].
 */
//case class NFNServer(maybeNFNNodeConfig: Option[RouterConfig], maybeComputeNodeConfig: Option[ComputeNodeConfig]) extends Actor {
case class NFNServer(nfnNodeConfig: RouterConfig, computeNodeConfig: ComputeNodeConfig) extends Actor {

  val logger = Logging(context.system, this)

  val ccnIf = CCNLite

  val cacheContent: Boolean = true

  val computeServer: ActorRef = context.actorOf(Props(classOf[ComputeServer]), name = "ComputeServer")

  val maybeLocalAbstractMachine: Option[ActorRef] =
      if(computeNodeConfig.withLocalAM)
        Some(context.actorOf(Props(classOf[LocalAbstractMachineWorker], self), "LocalAM"))
      else None

  val defaultTimeoutDuration = StaticConfig.defaultTimeoutDuration

  var pit: ActorRef = context.actorOf(Props(classOf[PIT]), name = "PIT")
  var cs: ActorRef = context.actorOf(Props(classOf[ContentStore]), name = "ContentStore")

  val nfnGateway: ActorRef =
      context.actorOf(Props(classOf[UDPConnectionContentInterest],
        new InetSocketAddress(computeNodeConfig.host, computeNodeConfig.port),
        new InetSocketAddress(nfnNodeConfig.host, nfnNodeConfig.port)),
        name = s"udpsocket-${computeNodeConfig.host}-${nfnNodeConfig.port}")

  override def preStart() = {
      nfnGateway ! UDPConnection.Handler(self)
  }

  // Check pit for an address to return content to, otherwise discard it
  private def handleContent(content: Content, senderCopy: ActorRef) = {

    def handleInterstThunkContent = {
      def timeoutFromContent: FiniteDuration = {
        val timeoutInContent = new String(content.data)
        if(timeoutInContent != "" && timeoutInContent.forall(_.isDigit)) {
          timeoutInContent.toInt.seconds
        } else {
          defaultTimeoutDuration
        }
      }

      implicit val timeout = Timeout(timeoutFromContent)
      (pit ? PIT.Get(content.name)).mapTo[Option[Set[Face]]] onSuccess {
          case Some(pendingFaces) => {
            val (contentNameWithoutThunk, isThunk) = content.name.withoutThunkAndIsThunk

            assert(isThunk, s"handleInterstThunkContent received the content object $content which is not a thunk")

            val interest = Interest(contentNameWithoutThunk)
            logger.debug(s"Received usethunk $content, sending actual interest $interest")

            logger.debug(s"Timeout duration: ${timeout.duration}")
            pendingFaces foreach { pf => pit ! PIT.Add(contentNameWithoutThunk, pf, timeout.duration) }
            nfnGateway ! interest
              // else it was a thunk interest, which means we can now send the actual interest
            pit ! PIT.Remove(content.name)
          }
          case None => logger.error(s"Discarding thunk content $content because there is no entry in pit")
        }
    }
    def handleNonThunkContent = {

      implicit val timeout = Timeout(defaultTimeoutDuration)
      (pit ? PIT.Get(content.name)).mapTo[Option[Set[Face]]] onSuccess {
        case Some(pendingFaces) => {

          if (cacheContent) {
            cs ! ContentStore.Add(content)
          }

          pendingFaces foreach { pendingSender => pendingSender.send(content) }

          pit ! PIT.Remove(content.name)
        }
        case None =>
          logger.warning(s"Discarding content $content because there is no entry in pit")
      }
    }

    if(content.name.isThunk && !content.name.isCompute) {
      handleInterstThunkContent
    } else {
      handleNonThunkContent
    }
  }

  private def handleInterest(i: Interest, senderCopy: ActorRef) = {

    implicit val timeout = Timeout(defaultTimeoutDuration)
    val f: Future[Unit] =
    (cs ? ContentStore.Get(i.name)).mapTo[Option[Content]] map {
      case Some(contentFromLocalCS) =>
        logger.debug(s"Served $contentFromLocalCS from local CS")
        senderCopy ! contentFromLocalCS
      case None => {
        val senderFace = ActorRefFace(senderCopy)
        (pit ? PIT.Get(i.name)).mapTo[Option[Set[Face]]] onSuccess {
          case Some(pendingFaces) =>
            pit ! PIT.Add(i.name, senderFace, defaultTimeoutDuration)
          case None => {
            pit ! PIT.Add(i.name, senderFace, defaultTimeoutDuration)

            // /.../.../NFN
            // nfn interests are either:
            // - send to the compute server if they start with compute
            // - send to a local AM if one exists
            // - forwarded to nfn gateway
            // not nfn interests are always forwarded to the nfn gateway
            if (i.name.isNFN) {
              // /COMPUTE/call .../.../NFN
              // A compute flag at the beginning means that the interest is a binary computation
              // to be executed on the compute server
              if (i.name.isCompute) {
                if(i.name.isThunk) {
                  computeServer ! ComputeServer.Thunk(i.name)
                } else {
                  computeServer ! ComputeServer.Compute(i.name)
                }
                // /.../.../NFN
                // An NFN interest without compute flag means that it must be reduced by an abstract machine
                // If no local machine is available, forward it to the nfn network
              } else {
                maybeLocalAbstractMachine match {
                  case Some(localAbstractMachine) => {
                    localAbstractMachine ! i
                  }
                  case None => {
                    nfnGateway ! i
                  }
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

  def handleNack(nack: NAck, senderCopy: ActorRef) = {
    if(StaticConfig.isNackEnabled) {
      implicit val timeout = Timeout(defaultTimeoutDuration)
      (pit ? PIT.Get(nack.name)).mapTo[Option[Set[Face]]] onSuccess {
        case Some(pendingFaces) => {
          pendingFaces foreach {
            _.send(nack)
          }
          pit ! PIT.Remove(nack.name)
        }
        case None => logger.warning(s"Received nack for name which is not in PIT: $nack")
      }
    }else{
      logger.error(s"Received nack even though nacks are disabled!")
    }
  }

  def handlePacket(packet: CCNPacket, senderCopy: ActorRef) = {
//    def monitorReceive(packet: CCNPacket): Unit = {
//      val ccnPacketLog = packet match {
//        case i: Interest => Monitor.InterestInfoLog("interest", i.name.toString)
//        case c: Content => {
//          val name = c.name.toString
//          val data = new String(c.data).take(50)
//          Monitor.ContentInfoLog("content", name, data)
//        }
//      }
//      Monitor.monitor ! new Monitor.PacketLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog, isSent = false, ccnPacketLog)
//    }

    packet match {
      case i: Interest => {
        logger.info(s"Received interest: $i")
        handleInterest(i, senderCopy)
      }
      case c: Content => {
        logger.info(s"Received content: $c")
        handleContent(c, senderCopy)
      }
      case n: NAck => {
        logger.info(s"Received NAck: $n")
        handleNack(n, senderCopy)
      }
    }
  }

  override def receive: Actor.Receive = {
    // received Data from network
    // If it is an interest, start a compute request
    case packet:CCNPacket => handlePacket(packet, sender)
    case UDPConnection.Received(data, sendingRemote) => {
      val maybePacket = byteStringToPacket(data)
      maybePacket match {
        // Received an interest from the network (byte format) -> spawn a new worker which handles the messages (if it crashes we just assume a timeout at the moment)
        case Some(packet: CCNPacket) => handlePacket(packet, sender)
        case Some(AddToCache()) => ???
        case None => {
          if(new String(data).contains("Content successfully added")) {
            logger.debug(s"Received content add to cache ack")
          } else {
            val failedMessagesDir = "./failed-messages"
            val failedMessagesDirFile= new File(failedMessagesDir)

            if(!failedMessagesDirFile.exists) {
              failedMessagesDirFile.mkdir
            }
            val f = new File(s"$failedMessagesDir/${System.nanoTime}")
            val fos = new FileOutputStream(f)
            fos.write(data)
            fos.close()
            logger.error(s"Received data which cannot be parsed to a ccn packet: ${new String(data)}")
          }
        }
      }
    }

    /**
     * [[NFNApi.CCNSendReceive]] is a message of the external API which retrieves the content for the interest and sends it back to the sender actor.
     * The sender actor can be initialized from an ask patter or form another actor.
     * It tries to first serve the interest from the localAbstractMachine cs, otherwise it adds an entry to the pit
     * and asks the network if it was the first entry in the pit.
     * Thunk interests get converted to normal interests, thunks need to be enabled with the boolean flag in the message
     */
    case NFNApi.CCNSendReceive(interest, useThunks) => {
      val maybeThunkInterest =
        if(interest.name.isNFN && useThunks) interest.thunkify
        else interest
      handlePacket(maybeThunkInterest, sender)
    }

    case NFNApi.AddToCCNCache(content) => {
      logger.info(s"sending add to cache for $content")
      nfnGateway ! UDPConnection.Send(ccnIf.mkAddToCacheInterest(content))
    }

    case NFNApi.AddToLocalCache(content, prependLocalPrefix) => {
      val contentToAdd =
        if(prependLocalPrefix) {
          Content(computeNodeConfig.prefix.append(content.name), content.data)
        } else content
      logger.info(s"Adding content for ${contentToAdd.name} to local cache")
      cs ! ContentStore.Add(contentToAdd)
    }


    case Exit() => {
      exit()
      context.system.shutdown()
    }
  }

  def exit(): Unit = {
    computeServer ! PoisonPill
    nfnGateway ! PoisonPill
  }
}
