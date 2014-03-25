package network

import akka.actor._
import akka.event.Logging
import java.net.InetSocketAddress
import akka.io.{Udp, IO}
import akka.util.ByteString

import scala.concurrent._
import ExecutionContext.Implicits.global

case class Send(data: Array[Byte])

class UDPConnection(worker: ActorRef,
                    target:InetSocketAddress = new InetSocketAddress("localhost", 9001),
                    remote:InetSocketAddress = new InetSocketAddress("localhost", 9000)) extends Actor {
  import context.system

  val name = self.path.name

  val logger = Logging(context.system, this)

  override def preStart() = {
    // IO is the manager of the akka IO layer, send it a request
    // to listen on a certain host on a port
    IO(Udp) ! Udp.Bind(self, target)
  }

  def receive = {
    // Received udp listener actor, set it as own actor
    case Udp.Bound(local) =>
      logger.info(s"$name ready")
      context.become(ready(sender))
    case Udp.CommandFailed(cmd) =>
      logger.error(s"Udp command '$cmd' failed")
  }

  def ready(socket: ActorRef): Receive = {
    case Send(data) => {
      logger.debug(s"$name sending data: ${new String(data)}")
      socket ! Udp.Send(ByteString(data), remote)
    }
    case Udp.Received(data, sendingRemote) => {
      logger.debug(s"$name received ${data.decodeString("utf-8")})")
      worker ! data
    }
    case Udp.Unbind  => socket ! Udp.Unbind
    case Udp.Unbound => context.stop(self)
  }
}

//class UDPSender(host: String = "localhost", port: Int = 9000) extends Actor {
//  import context.system
//  val remote = new InetSocketAddress(host, port)
//
//  override def preStart() = {
//    IO(Udp) ! Udp.SimpleSender
//  }
//
//  def receive = {
//    case Udp.SimpleSenderReady =>
//      context.become(ready(sender))
//  }
//
//  def ready(send: ActorRef): Receive = {
//    case Send(data) =>
//      println(s"UDPSender: sent ${new String(data)}")
//      send ! Udp.Send(ByteString(data), remote)
//  }
//}