package monitor

import akka.actor.{Actor, ActorSystem, Props}
import ccn.packet.{CCNPacket, Packet}
import config.AkkaConfig
import java.net.InetSocketAddress
import network.UDPConnection
import nfn.NodeConfig
import akka.util.ByteString


object Monitor {

  val host = "localhost"
  val port = 10666

  private val system = ActorSystem(s"Monitor", AkkaConfig())

  val monitor = system.actorOf(Props(classOf[Monitor]))

  case class Connect(from: NodeConfig, to: NodeConfig)
  case class Disconnect(from: NodeConfig, to: NodeConfig)

  case class PacketReceived(packet: Packet, receiver: NodeConfig)

  case class PacketSent(packet: Packet, sender: NodeConfig)

  case class Visualize()

  case class Send(packet: Packet)
}

/**
 * Created by basil on 17/04/14.
 */
case class Monitor() extends Actor {

  val system = ActorSystem(s"Monitor", AkkaConfig())
  val nfnSocket = system.actorOf(
    Props(classOf[UDPConnection],
    new InetSocketAddress(Monitor.host, Monitor.port),
    None),
    name = s"udpsocket-${Monitor.port}"
  )


  var nodes = Set[NodeConfig]()

  var edges = Set[Pair[NodeConfig, NodeConfig]]()

  var packetsSent = Set[(Packet, NodeConfig)]()
  var packetsReceived = Set[(Packet, NodeConfig)]()

  var packetsMaybeSentMaybeReceived = Map[Packet, Pair[Option[NodeConfig], Option[NodeConfig]]]()

  override def receive: Receive = {
    case data: Array[Byte] => {
    }
    case data: ByteString => {
    }
    case Monitor.Connect(from, to) => {
      if(!edges.contains(Pair(from, to))) {
        nodes += to
        nodes += from
        edges += Pair(from, to)
      }
    }
    case Monitor.Disconnect(from, to) =>
    case Monitor.Visualize() => {
      new GraphFrame(nodes, edges, packetsMaybeSentMaybeReceived)
    }
    case Monitor.PacketSent(packet, sndr) => {
      packetsMaybeSentMaybeReceived += (
        packet -> (
          if(packetsMaybeSentMaybeReceived.contains(packet)) {
            packetsMaybeSentMaybeReceived(packet) match {
              case Pair(None, rcvr @ _) => Pair(Some(sndr), rcvr)
              case p @ Pair(Some(_), _) => println("ERROR: this packet was already sent!"); p
            }
          } else {
            Pair(Some(sndr), None)
          })
      )
    }
    case Monitor.PacketReceived(packet, rcvr) => {
      packetsMaybeSentMaybeReceived += (
        packet -> (
          if(packetsMaybeSentMaybeReceived.contains(packet)) {
            packetsMaybeSentMaybeReceived(packet) match {
              case Pair(sndr @ _, None) => Pair(sndr, Some(rcvr))
              case p @ Pair(_, Some(_)) => println("ERROR: this packet was already received!"); p
            }
          } else {
            Pair(None, Some(rcvr))
          })
        )
    }
  }
}




//case class GraphPosition(x: Float, y: Float)
//
//trait GraphPositional {
//  val r = new Random()
//  var pos = GraphPosition(r.nextFloat, r.nextFloat)
//
//}
//
//case class SpecificGraphPosition(x: Float, y: Float) extends GraphPositional {
//  override def pos: GraphPosition = GraphPosition(x, y)
//}



