package network

import akka.actor._
import akka.event.Logging
import java.net.InetSocketAddress
import akka.io.{Udp, IO}
import akka.util.ByteString

import scala.concurrent._
import ExecutionContext.Implicits.global
import network.UDPConnection._

object UDPConnection {
  case class Send(data: Array[Byte])
  case class Received(data: Array[Byte], sendingRemote: InetSocketAddress)
  case class Handler(worker: ActorRef)
}

/**
 * A connection between a target socket and a remote socket.
 * Data is sent by passing an [[network.UDPConnection.Send]] message containing the data to be sent.
 * Receiving data is handled by the member worker actor.
 * This actor initializes itself on preStart by sending a bind message to the IO manager.
 * On receiving a Bound message, it sets the ready method s its new [[akka.actor.Actor.Receive]] handler,
 * which passes recieved data to the worker and sends data to the remote.
 *
 * @param local Socket to listen for data
 * @param maybeTarget If Some(addr), the connection sends data to the target on receiving [[Send]] messages
 */
class UDPConnection(local:InetSocketAddress, maybeTarget:Option[InetSocketAddress]) extends Actor {
  import context.system

  val name = self.path.name

  val logger = Logging(context.system, this)

  private var workers: List[ActorRef] = Nil

  private var notConnectedSendQueue = List[Send]()

  override def preStart() = {
    // IO is the manager of the akka IO layer, send it a request
    // to listen on a certain host on a port
    IO(Udp) ! Udp.Bind(self, local)
  }

  def handleWorker(worker: ActorRef) = {
    logger.debug(s"Registered worker $worker")
    workers ::= worker
  }

  def receive: Receive = {
    // Received udp socket actor, change receive handler to ready mehtod with reference to the socket actor
    case Udp.Bound(local) =>
      logger.info(s"$name ready")
      if(notConnectedSendQueue.size > 0 ) {
        logger.info(s"Sending ${notConnectedSendQueue.size} queued messages")
      }
      context.become(ready(sender))
      notConnectedSendQueue foreach { self ! _ }
      notConnectedSendQueue = Nil
    case Udp.CommandFailed(cmd) =>
      logger.error(s"Udp command '$cmd' failed")
    case send: Send => {
      logger.debug(s"Adding to queue")
      if(notConnectedSendQueue.size > 10) logger.warning("More than 10 messages queued and still not connected")
      notConnectedSendQueue ::= send
    }
    case Handler(worker) => handleWorker(worker)
  }

  def ready(socket: ActorRef): Receive = {
    case Send(data) => {
      maybeTarget match {
        case Some(target) => {
          logger.debug(s"$name sending data")
          socket ! Udp.Send(ByteString(data), target)
        }
        case None => logger.warning("Received Send message, but target was not configurated on init, this socket is not able to send!")
      }
    }
    case Udp.Received(data, sendingRemote) => {
      logger.debug(s"$name received ${data.decodeString("utf-8")})")
      frowardReceivedData(data, sendingRemote)
    }
    case Udp.Unbind  => socket ! Udp.Unbind
    case Udp.Unbound => context.stop(self)
    case Handler(worker) => handleWorker(worker)
  }

  def frowardReceivedData(data: ByteString, sendingRemote: InetSocketAddress): Unit = {
    workers.foreach(_ ! Received(data.toByteBuffer.array.clone(), sendingRemote))
  }
}

case class UDPSender(remote: InetSocketAddress) extends Actor {
  import context.system

  val logger = Logging(context.system, this)

  override def preStart() = {
    IO(Udp) ! Udp.SimpleSender
  }


  override def receive: Actor.Receive = {
    case Udp.SimpleSenderReady => {
      logger.debug("ready")
      context.become(ready(sender))
    }
  }

  def ready(socket: ActorRef): Actor.Receive = {
    case data: Array[Byte] => {
      logger.debug(s"Sending data")
      socket ! Udp.Send(ByteString(data), remote)
    }
  }
}

