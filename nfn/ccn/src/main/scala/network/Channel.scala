package network

import akka.actor.{Props, ActorSystem, ActorRef, Actor}

trait Packet

trait NetworkPacket
case class Receive(dataPacket: Packet) extends NetworkPacket
case class Connection(actor: ActorRef) extends NetworkPacket
case class Send(dataPacket: Packet) extends NetworkPacket

trait Face extends Actor {
  def name: String
}

//abstract class ActorFace extends Actor with Face {


//  def name: String
//
//  def receive(packet: Packet, in: ActorRef)
//
//  def beforeSend(packet: Packet)
//
//  private var maybeOut: Option[ActorRef] = None
//
//  def receive: Actor.Receive = {
//    case Connection(actor) => {
//      maybeOut = Some(actor)
//      afterConnect(self, actor)
//    }
//    case Receive(packet) => {
//      receive(packet, sender)
//    }
//    case Send(packet) => {
//      send(packet)
//    }
//  }
//
//  def out():ActorRef = maybeOut match {
//    case Some(out) => out
//    case None => throw new Exception("interface not connected to channel")
//  }
//
//  def send(packet: Packet): Unit = {
//    println(s"$name sending $packet to $out")
//    beforeSend(packet)
//    out ! Receive(packet)
//  }
//}
//
//case class PrintSocket(override val name: String) extends ActorFace {
//  def receive(p: Packet, in: ActorRef): Unit = {
//    println(s"$name received: " + p)
//  }
//
//  def beforeSend(packet: Packet): Unit = {
//    println(s"$name sent: " + packet)
//  }
//
//  def connectTo(other: ActorRef): Unit = {}
//
//  def afterConnect(self: ActorRef, other: ActorRef): Unit = {
//
//  }
//}
//
//object Network {
//  def test() = {
//    val system = ActorSystem("system")
//
//    val leftActor = system.actorOf(Props[ActorFace](new PrintSocket("LEFT")))
//
//    val rightActor = system.actorOf(Props[ActorFace](new PrintSocket("RIGHT")))
//
//    rightActor ! Connection(leftActor)
//    leftActor ! Connection(rightActor)
//  }
//}