package nfn

import akka.actor._
import akka.util.ByteString
import com.typesafe.scalalogging.slf4j.Logging

import network.{NFNCommunication, Send, UDPConnection}
import scala.util.{Failure, Success}
import ccn.packet._
import akka.event.Logging
import nfn.service._
import ccn.ccnlite.CCNLite

/**
 * Worker Actor which responds to ccn interest and content packets
 */
abstract class CCNWorker() extends Actor {

  val logger = Logging(context.system, this)
  val name = self.path.name

  val ccnIf = CCNLite

  def receivedContent(content: Content)

  def receivedInterest(interest: Interest)

  override def receive: Actor.Receive = {
    case data: ByteString =>
      val byteArr = data.toByteBuffer.array.clone
      logger.info(s"$name: received ${new String(byteArr)}")
      NFNCommunication.parseXml(ccnIf.ccnbToXml(byteArr)) match {
        case c: Content => receivedContent(c)
        case i: Interest => receivedInterest(i)
      }
  }
}

/**
 * Worker for the compute Socket
 * @param ccnSocket Actor reference to a ccn socket
 */
case class ComputeWorker(ccnSocket: ActorRef) extends CCNWorker() {

  /**
   * On interest receive, finds the corresponding local service for the given interest name,
   * then initialized this service instance with the arguments in the interest name.
   * Finally execute the service, packs the result into a content object and sends it to ccn.
   *
   * If there is any error, simply log an error and do nothing. This will result in a timeout in the network.
   * @param interest
   */
  override def receivedInterest(interest: Interest): Unit = {
    val cmps = interest.name
    val computeCmps = cmps.slice(1, cmps.size-1)
    NFNService.parseAndFindFromName(computeCmps.mkString(" ")) flatMap { _.exec } match {
      case Success(nfnServiceValue) => nfnServiceValue match {
        case NFNIntValue(n) => {
          val content = Content(cmps, n.toString.getBytes)
          val binaryContent = ccnIf.mkBinaryContent(content)
          ccnSocket ! Send(binaryContent)
        }
        case res @ _ => logger.error(s"Compute Server response is only implemented for results of type NFNIntValue and not: $res")
      }
      case Failure(e) => logger.error(s"There was an error when looking for a service: \n${e.getStackTrace.mkString("/n")}\n")
    }
  }

  /**
   * Compute socket receiving a content object is undefined. Currently discards the packet.
   * @param content
   */
  override def receivedContent(content: Content): Unit = {
    logger.warning(s"Compute socket received content packet $content, discarding it"); None
  }
}

/**
 *
 */
case class NFNWorker() extends CCNWorker() {

  override def receivedContent(content: Content) = {
    val name = content.name.mkString("/")
    val data = content.data

    val dataString = new String(data)
    val resultPrefix = "RST|"

    val resultContentString = dataString.startsWith(resultPrefix) match {
      case true => dataString.substring(resultPrefix.size)
      case false => throw new Exception(s"NFN could not compute result for: $name")
    }
    logger.info(s"$name result: '$name' => '$resultContentString'")
    resultContentString
  }

  override def receivedInterest(interest: Interest) = {
    logger.warning(s"$name received interest '$interest', discarding it")
  }

}

