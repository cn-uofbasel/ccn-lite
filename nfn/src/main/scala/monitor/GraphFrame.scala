package monitor

import nfn.NodeConfig
import com.mxgraph.view.mxGraph
import javax.swing.JFrame
import scala.util.Random
import com.mxgraph.layout.mxFastOrganicLayout
import com.mxgraph.swing.mxGraphComponent
import com.mxgraph.util.{mxEventObject, mxEvent}
import com.mxgraph.swing.util.mxMorphing
import com.mxgraph.util.mxEventSource.mxIEventListener

/**
 * Created by basil on 17/04/14.
 */
class GraphFrame(nodes: Set[NodeConfig], edges: Set[Pair[NodeConfig, NodeConfig]]) extends JFrame {

  val visualizingGraph = new mxGraph()

  val par = visualizingGraph.getDefaultParent
  visualizingGraph.getModel.beginUpdate()

  val nodesWithVertizes: Map[NodeConfig, AnyRef] = nodes.map({ node =>
    val r = new Random()
    val x = r.nextFloat()
    val y = r.nextFloat()
//    val width = 300f
    val width = 0f
//    val height = 300f
    val height = 0f
    val v = visualizingGraph.insertVertex(par, null, node, x * width, y*height, 50, 50)
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


  val graphComponent = new mxGraphComponent(visualizingGraph)
  // layout using morphing
  visualizingGraph.getModel().beginUpdate()
  graphLayout.execute(par)
  val morph = new mxMorphing(graphComponent, 6, 1.5, 20)
  morph.addListener(mxEvent.DONE, new mxIEventListener() {
    @Override
    def invoke(arg0: AnyRef, arg1: mxEventObject) {
      visualizingGraph.getModel().endUpdate();

    }
  })

  morph.startAnimation()

  getContentPane.add(graphComponent)

  setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE)
  setSize(400, 320)
  setVisible(true)
}
