package monitor

import akka.actor.{Actor, ActorSystem, Props}
import ccn.packet.CCNPacket
import config.AkkaConfig
import java.net.InetSocketAddress
import network.UDPConnection
import nfn.{NFNMaster, NodeConfig}
import akka.util.ByteString
import akka.event.Logging


object Monitor {

  val host = "localhost"
  val port = 10666

  private val system = ActorSystem(s"Monitor", AkkaConfig())

  val monitor = system.actorOf(Props(classOf[Monitor]))

  case class Connect(from: NodeConfig, to: NodeConfig)
  case class Disconnect(from: NodeConfig, to: NodeConfig)

  case class PacketReceived(packet: CCNPacket, receiver: NodeConfig)

  case class PacketSent(packet: CCNPacket, sender: NodeConfig)

  case class Visualize()

  case class Send(packet: CCNPacket)
}

/**
 * Created by basil on 17/04/14.
 */

trait LogEntry {
  def timestamp: Long
}

case class CCNPacketReceiveLog(packet: CCNPacket, receiver: NodeConfig, timestamp: Long) extends LogEntry
case class CCNPacketSentLog(packet: CCNPacket, sender: NodeConfig, timestamp: Long) extends LogEntry

case class Monitor() extends Actor {
  val logger = Logging(context.system, this)

  val system = ActorSystem(s"Monitor", AkkaConfig())
  val nfnSocket = system.actorOf(
    Props(classOf[UDPConnection],
    new InetSocketAddress(Monitor.host, Monitor.port),
    None),
    name = s"udpsocket-${Monitor.port}"
  )


  var nodes = Set[NodeConfig]()

  var edges = Set[Pair[NodeConfig, NodeConfig]]()

//  var packetsSent = Set[(Packet, NodeConfig)]()
//  var packetsReceived = Set[(Packet, NodeConfig)]()

  var packetsSent= Map[Seq[String], CCNPacketSentLog]()
  var packetsReceived = Map[Seq[String], CCNPacketReceiveLog]()

  override def receive: Receive = {
    case UDPConnection.Received(data, sendingRemote) => {
      NFNMaster.byteStringToPacket(data) match {
        case Some(packet) => {
          nodes.find(nodeConfig => nodeConfig.host == sendingRemote.getHostName && nodeConfig.port == sendingRemote.getPort) match {
            case Some(nodeConfig) => {
              packet match {
                case ccnPacket: CCNPacket =>
                  self ! Monitor.PacketSent(ccnPacket, nodeConfig)
                case _ => logger.warning(s"Received packet $packet, discarding it")
              }
            }
            case None => logger.warning(s"Received packet $packet from node $sendingRemote, but this node was never added")
          }
        }
        case None => logger.debug("could not parse received packet (probably add to cache)")
      }
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
      new GraphFrame(nodes, edges, packetsSent, packetsReceived)
    }
    case Monitor.PacketSent(packet, sndr) => {
      packet match {
        case ccnPacket: CCNPacket =>
          packetsSent += (ccnPacket.name -> CCNPacketSentLog(ccnPacket, sndr, System.currentTimeMillis))
        case _ => logger.warning(s"Sent packet $packet was discarded and not logged")
      }
    }

    case Monitor.PacketReceived(packet, rcvr) => {
      packet match {
        case ccnPacket: CCNPacket =>
          packetsReceived += (ccnPacket.name -> CCNPacketReceiveLog(ccnPacket, rcvr, System.currentTimeMillis))
        case _ => logger.warning(s"Recieved packet $packet was discarded and not logged")
      }
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



