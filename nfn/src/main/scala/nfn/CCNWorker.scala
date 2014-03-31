package nfn

import akka.actor._
import akka.util.ByteString
import akka.event.Logging

import network._
import network.NetworkConnection._
import nfn.service._
import ccn.ccnlite.CCNLite
import ccn.packet._
import nfn.CCNWorker._
import scala.concurrent.ExecutionContext.Implicits.global

trait CCNPacketHandler {
  def receivedContent(content: Content): Unit

  def receivedInterest(interest: Interest): Unit
}

object CCNWorker {
  case class CCNSendReceive(interest: Interest)
  case class CCNSend(interest: Interest)
  case class CCNAddToCache(content: Content)
  case class ComputeResult(content: Content)
}
/**
 * Worker Actor which responds to ccn interest and content packets
 */
case class CCNWorker() extends Actor {

  val nfnSocket = context.actorOf(Props(new NetworkConnection()), name = "udpsocket")

  val logger = Logging(context.system, this)
  val name = self.path.name

  val ccnIf = CCNLite

  var pendingInterests: Map[Seq[String], ActorRef] = Map()

  override def preStart() = {
    nfnSocket ! Handler(self)
  }

  private def createComputeWorker(interest: Interest): ActorRef =
    context.actorOf(Props(classOf[ComputeWorker], self), s"ComputeWorker-${interest.name.mkString("/").hashCode}")

  override def receive: Actor.Receive = {

    // received Data from network
    // If it is an interest, start a compute request
    case data: ByteString => {
      val byteArr = data.toByteBuffer.array.clone
      val packet = NFNCommunication.parseXml(ccnIf.ccnbToXml(byteArr))

      logger.info(s"$name received $packet")
      packet match {
        // Received an interest from the network (byte format) -> spawn a new worker which handles the messages (if it crashes we just assume a timeout at the moment)
        case interest: Interest => {
          val computeWorker = createComputeWorker(interest)
          pendingInterests += (interest.name -> computeWorker)
          computeWorker.tell(interest, self)
        }
        // Received content, check if there is an entry in the pit. If so, send it to the attached worker, otherwise discard it
        case content: Content => {
          pendingInterests.get(content.name) match {
            case Some(interestSender) => interestSender ! content
            case None => logger.error(s"Discarding content $content without pending interest")
          }
        }
      }
    }

    case CCNSendReceive(interest) => {
      logger.info(s"$name received send-recv request for interest: $interest")
      pendingInterests += (interest.name -> sender)
      val binaryInterest = ccnIf.mkBinaryInterest(interest)
      nfnSocket ! Send(binaryInterest)
    }

    // CCN worker itself is responsible to handle compute requests from the network (interests which arrived in binary format)
    // convert the result to ccnb format and send it to the socket
    case ComputeResult(content) => {
      pendingInterests.get(content.name) match {
        case Some(_) => {
          logger.debug("sending compute result to socket")
          val binaryContent = ccnIf.mkBinaryContent(content)
          nfnSocket ! Send(binaryContent)
        }
        case None => logger.error(s"Received result from compute worker which timed out, discarding the result content: $content")
      }
    }

    case CCNAddToCache(content) => {
      val binaryInterest = ccnIf.mkAddToCacheInterest(content)
      nfnSocket ! Send(binaryInterest)
    }
  }
}


/**
 *
 */
case class ComputeWorker(ccnWorker: ActorRef) extends Actor {

  val name = self.path.name
  val logger = Logging(context.system, this)
  val ccnIf = CCNLite

  private var result : Option[String] = None

  def receivedContent(content: Content) = {
    // Received content from request (sendrcv)
    ???
    logger.error(s"ComputeWorker received content, discarding it because it does not know what to do with it")
  }

  // Recievied compute request
  // Make sure it actually is a compute request and forward to the handle method
  def receivedInterest(interest: Interest, requestor: ActorRef) = {
    logger.debug(s"received compute interest: $interest")
    val cmps = interest.name
    val computeCmps = cmps match {
      case Seq("COMPUTE", reqNfnCmps @ _ *) => {
        val computeCmps = reqNfnCmps.take(reqNfnCmps.size - 1)
        handleComputeRequest(computeCmps, interest, requestor)
      }
      case _ => logger.error(s"Dropping interest $interest because it is not a compute request")
    }
  }

  def handleComputeRequest(computeCmps: Seq[String], interest: Interest, requestor: ActorRef) = {
    val futResultContent =
      for{callableServ <-  NFNService.parseAndFindFromName(computeCmps.mkString(" "), ccnWorker)
    } yield {
      // TODO fix the issue of name components vs single names...
      logger.warning(s"TODO: fix the issue of name components vs single names...")
      val servResult = callableServ.exec
      Content(interest.name, servResult.toValueName.name.mkString(" ").getBytes)
    }
    futResultContent onSuccess {
      case content => {
        logger.debug(s"Finished computation, result: $content")
        requestor ! ComputeResult(content)
      }
    }
  }

  override def receive: Actor.Receive = {
    case content: Content => receivedContent(content)
    case interest: Interest => {
      // Just to make sure we are not closing over sender
      val requestor = sender
      receivedInterest(interest, requestor)
    }
  }
}
