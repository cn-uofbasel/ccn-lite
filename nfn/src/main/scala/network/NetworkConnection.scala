package network

import akka.actor._
import akka.event.Logging
import java.net.InetSocketAddress
import akka.io.{Udp, IO}
import akka.util.ByteString

import scala.concurrent._
import ExecutionContext.Implicits.global
import network.NetworkConnection._

object NetworkConnection {
  case class Send(data: Array[Byte])
  case class Handler(worker: ActorRef)
}

/**
 * A connection between a target socket and a remote socket.
 * Data is sent by passing an [[network.NetworkConnection.Send]] message containing the data to be sent.
 * Receiving data is handled by the member worker actor.
 * This actor initializes itself on preStart by sending a bind message to the IO manager.
 * On receiving a Bound message, it sets the ready method s its new [[akka.actor.Actor.Receive]] handler,
 * which passes recieved data to the worker and sends data to the remote.
 *
 * @param local Socket to listen for data
 * @param target socket to sent data to
 */
class NetworkConnection(local:InetSocketAddress = new InetSocketAddress("localhost", 9001),
                        target:InetSocketAddress = new InetSocketAddress("localhost", 9000)) extends Actor {
  import context.system

  val name = self.path.name

  val logger = Logging(context.system, this)

  private var workers: List[ActorRef] = Nil

  override def preStart() = {
    // IO is the manager of the akka IO layer, send it a request
    // to listen on a certain host on a port
    IO(Udp) ! Udp.Bind(self, local)
  }

  def receive: Receive = {
    // Received udp socket actor, change receive handler to ready mehtod with reference to the socket actor
    case Udp.Bound(local) =>
      logger.info(s"$name ready")
      context.become(ready(sender))
    case Udp.CommandFailed(cmd) =>
      logger.error(s"Udp command '$cmd' failed")
    case Handler(worker) => workers ::= worker
  }

  def ready(socket: ActorRef): Receive = {
    case Send(data) => {
      logger.debug(s"$name sending data")//${new String(data)}")
      socket ! Udp.Send(ByteString(data), target)
    }
    case Udp.Received(data, sendingRemote) => {
      logger.debug(s"$name received ${data.decodeString("utf-8")})")
      frowardReceivedData(data)
    }
    case Udp.Unbind  => socket ! Udp.Unbind
    case Udp.Unbound => context.stop(self)
    case Handler(worker) => workers ::= worker
  }

  def frowardReceivedData(data: ByteString): Unit = {
    workers.foreach(_ ! data)
  }
}


