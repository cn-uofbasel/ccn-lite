package monitor

import akka.actor.{Actor, ActorSystem, Props}
import ccn.packet.Packet
import config.AkkaConfig
import java.net.InetSocketAddress
import network.UDPConnection
import nfn.NodeConfig
import com.mxgraph.view.mxGraph
import javax.swing.JFrame
import scala.util.Random
import com.mxgraph.swing.mxGraphComponent
import com.mxgraph.layout.mxFastOrganicLayout
import akka.util.ByteString


object Monitor {
  val host = "localhost"
  val port = 10666

  private val system = ActorSystem(s"Monitor", AkkaConfig())

  val monitor = system.actorOf(Props(classOf[Monitor]))

  case class Connect(from: NodeConfig, to: NodeConfig)
  case class Disconnect(from: NodeConfig, to: NodeConfig)

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
      new GraphFrame(nodes, edges)
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



