import akka.actor.{ActorRef, ActorSystem, IOManager}
import akka.io.UdpConnected
import akka.io.UdpConnected.Connect
import akka.util.ByteString
import java.net.{DatagramPacket, DatagramSocket}

object UDPServer {
  def main(args: Array[String]) = {
    val serverSocket = new DatagramSocket(9000)

    val rcvData = new Array[Byte](1024)
    while(true) {
      val rcvPacket = new DatagramPacket(rcvData, rcvData.length)
      serverSocket.receive(rcvPacket)
      val reqStr = new String(rcvPacket.getData)
      println(s"server - rcv: $reqStr")

      val addr = rcvPacket.getAddress
      val port = rcvPacket.getPort

      val respData = rcvData
      val rspPacket = new DatagramPacket(respData, respData.length, addr, port)

      serverSocket.send(rspPacket)
    }
  }
}


import akka.actor.Actor
import akka.io.{Udp, IO}
import java.net.InetSocketAddress

/*
 * Simple unbound UDP sender (fire and forget) actor
 * To close it send a PoisonPill message.
 * For improved performance implement a bound UDP sender, which
 * does not need to perform certain security checks with each call
 */
class UDPSender(nextActor: ActorRef, port: Int = 9000, host: String = "localhost") extends Actor {
  import context.system

  val remote = new InetSocketAddress(host, port)

  // IO is the manager actor of the akka IO layer
  // Send a SimpleSender message to be notified with a ready message
  IO(Udp) ! Udp.SimpleSender

  def receive = {
    // UDP simple sender actor initialized by the IO manager
    // use the child actor as it's own actor with become
    case Udp.SimpleSenderReady =>
      context.become(ready(sender))
  }

  def ready(send: ActorRef): Receive = {
    case msg: String => send ! Udp.Send(ByteString(msg), remote)
    case data: Array[Byte] => send ! Udp.Send(ByteString(data), remote)
  }
}

case class Send(data: Array[Byte])

/**
 * UDP server
// * @param nextActor
 */
class UDPConnection(nextActor: ActorRef, port: Int = 9000, host: String = "localhost") extends Actor {
  import context.system

  val target = new InetSocketAddress(host, port)
  // IO is the manager of the akka IO layer, send it a request
  // to listen on a certain host on a port
  IO(Udp) ! Udp.Bind(self, target)

  def receive = {
    // Received udp listener actor, set it as own actor
    case Udp.Bound(local) =>
      println("UDPListener becomes ready")
      context.become(ready(sender))
    case Udp.CommandFailed(cmd) =>
      println(s"Udp command '$cmd' failed")
  }

  def ready(socket: ActorRef): Receive = {
    case Udp.Received(data, remote) => {
      println(s"Received: $data")
      nextActor ! data
    }
    case Udp.Unbind  => socket ! Udp.Unbind
    case Udp.Unbound => context.stop(self)

    case Send(data) =>
      println(s"sending data: $data")
      self ! Udp.Send(ByteString(data), target)
  }

  def send(data: Array[Byte]) = {
  }
}

class Connected(hostname: String, port: Int) extends Actor {
  import context.system

  val remote = new InetSocketAddress(hostname, port)
  IO(UdpConnected) ! UdpConnected.Connect(self, remote)

  def receive = {
    case UdpConnected.Connected =>
      context.become(ready(sender))
    case Udp.CommandFailed(cmd) =>
      println(s"Udp command '$cmd' failed")
  }

  def ready(connection: ActorRef): Receive = {
    case UdpConnected.Received(data) =>
    // process data, send it on, etc.
    case msg: String =>
      connection ! UdpConnected.Send(ByteString(msg))
    case d @ UdpConnected.Disconnect => connection ! d
    case UdpConnected.Disconnected   => context.stop(self)
  }
}

class NFNWorker extends Actor {
  def ccnIf = CCNLiteInterface

  override def receive: Actor.Receive = {
    case data: ByteString =>
      val byteArr = data.toByteBuffer.array.clone
      val unparsedXml = ccnIf.ccnbToXml(byteArr)
      NFNCommunication.parseXml(unparsedXml) match {
        case interest: Interest =>
          println(s"CLI RCV $interest")
        case content: Content =>
          println(s"CLI RCV $content")
      }
  }
}

import akka.actor._


object AkkaTest extends App {
  val system = ActorSystem("NFNActorSystem")

//  val worker = system.actorOf(Props[NFNWorker], name = "NFNWorker")
//  val udpConnection = system.actorOf(Props(new UDPConnection(worker)), name = "UDPListener")

//  val simpleSender = system.actorOf(Props(new SimpleUDPSender), name = "SimpleUDPSender")
//  Thread.sleep(2000)
//
//  val interest = "add 1 1/NFN"
//  udpConnection !  Send(new CCNLiteInterface().mkBinaryInterest(interest))

}


