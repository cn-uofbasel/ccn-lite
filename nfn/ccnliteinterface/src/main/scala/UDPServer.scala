import akka.actor.{ActorRef, ActorSystem, IOManager}
import akka.util.ByteString
import java.net.{DatagramPacket, DatagramSocket}

object UDPServer {
  def main(args: Array[String]) = {
    val serverSocket = new DatagramSocket(9876)

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
class SimpleUDPSender(port: Int = 9876, host: String = "localhost") extends Actor {
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

/**
 * UDP server
// * @param nextActor
 */
class UDPListener(nextActor: ActorRef, port: Int = 9876, host: String = "localhost") extends Actor {
  import context.system
  // IO is the manager of the akka IO layer, send it a request
  // to listen on a certain host on a port
  IO(Udp) ! Udp.Bind(self, new InetSocketAddress(host, port))

  def receive = {
    // Received udp listener actor, set it as own actor
    case Udp.Bound(local) =>
      println("UDPListener becomes ready")
      context.become(ready(sender))
  }

  def ready(socket: ActorRef): Receive = {
    case Udp.Received(data, remote) => nextActor ! data
    case Udp.Unbind  => socket ! Udp.Unbind
    case Udp.Unbound => context.stop(self)
  }
}

class NFNWorker extends Actor {
  def ccnIf = new CCNLiteInterface()

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

class TestClass() {
  this.getClass
  def bla = println(this.getClass.getProtectionDomain.getCodeSource.getLocation.getFile + this.getClass.getName + ".class")
}

object AkkaTest extends App {
  new TestClass().bla



//  val system = ActorSystem("NFNActorSystem")
//
//  val worker = system.actorOf(Props[NFNWorker], name = "NFNWorker")
//  val udpListener = system.actorOf(Props(new UDPListener(worker)), name = "UDPListener")
//
//  val simpleSender = system.actorOf(Props(new SimpleUDPSender), name = "SimpleUDPSender")
//
//  Thread.sleep(1000)
//  simpleSender ! new CCNLiteInterface().mkBinaryInterest("/test/interest")
}


