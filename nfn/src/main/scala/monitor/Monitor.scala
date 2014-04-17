package monitor

import akka.actor.{Actor, ActorSystem, Props}
import ccn.packet.Packet
import config.AkkaConfig
import java.net.InetSocketAddress
import network.UDPConnection
import nfn.{NFNMasterFactory, NodeConfig}
import scalax.collection.GraphEdge.DiEdge
import scalax.collection.mutable.Graph
import com.mxgraph.model.mxGraphModel
import com.mxgraph.view.mxGraph
import scala.collection.mutable
import javax.swing.JFrame
import scala.util.Random
import com.mxgraph.swing.mxGraphComponent
import com.mxgraph.layout.mxFastOrganicLayout


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


object TestApp extends App {
  val nodeConfs =
    Seq(
      NodeConfig("asdf1", 1, -1, "asdf"),
      NodeConfig("asdf2", 1, -1, "asdf"),
      NodeConfig("asdf3", 1, -1, "asdf"),
      NodeConfig("asdf4", 1, -1, "asdf")
    )

  val nodes = nodeConfs.toSet
  val edges = Set(
    Pair(nodeConfs(0), nodeConfs(1)),
    Pair(nodeConfs(1), nodeConfs(2)),
    Pair(nodeConfs(2), nodeConfs(3)),
    Pair(nodeConfs(3), nodeConfs(0))
  )

  new GraphFrame(nodes, edges)
}

class GraphFrame(nodes: Set[NodeConfig], edges: Set[Pair[NodeConfig, NodeConfig]]) extends JFrame {

  val visualizingGraph = new mxGraph()

  val par = visualizingGraph.getDefaultParent
  visualizingGraph.getModel.beginUpdate()

  val r = new Random()
  val nodesWithVertizes: Map[NodeConfig, AnyRef] = nodes.map({ node =>
    val v = visualizingGraph.insertVertex(par, null, node, 50 + r.nextInt(300), 50 + r.nextInt(300), 50, 50)
    node -> v
  }).toMap


  val edgesWithVertex = edges map { e =>
  edges foreach { edge =>
    val v1  = nodesWithVertizes(edge._1)
    val v2  = nodesWithVertizes(edge._2)
    visualizingGraph.insertEdge(par, null, "Edge", v1, v2)
  }

  visualizingGraph.getModel.endUpdate()
  }

  val graphLayout:  mxFastOrganicLayout = new mxFastOrganicLayout(visualizingGraph);

  graphLayout.setForceConstant(300) // the higher, the more separated
  // layout graph
  graphLayout.execute(par)

  val graphComponent = new mxGraphComponent(visualizingGraph)
  getContentPane.add(graphComponent)

  setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE)
  setSize(400, 320)
  setVisible(true)
}
