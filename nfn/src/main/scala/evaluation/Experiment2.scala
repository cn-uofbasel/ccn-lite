package evaluation

import scala.util._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.util.Timeout

import nfn._
import ccn.packet._
import scala.concurrent.Future
import node.Node
import monitor.Monitor
import lambdacalculus.parser.ast._
import nfn.service.impl._
import config.AkkaConfig

object Experiment2 extends App {


  val node1Config = NodeConfig("127.0.0.1", 10010, 10011, CCNName("node", "node1"))
  val node2Config = NodeConfig("127.0.0.1", 10020, 10021, CCNName("node", "node2"))
  val node3Config = NodeConfig("127.0.0.1", 10030, 10031, CCNName("node", "node3"))
  val node4Config = NodeConfig("127.0.0.1", 10040, 10041, CCNName("node", "node4"))

  val docname1 = node1Config.prefix.append("doc", "test1")
  val docdata1 = "one".getBytes
  val docContent1 = Content(docname1, docdata1)

  val docname2 = node2Config.prefix.append("doc", "test2")
  val docdata2 = "two two".getBytes
  val docContent2 = Content(docname2, docdata2)

  val docname3 = node3Config.prefix.append("doc", "test3")
  val docdata3 = "three three three".getBytes
  val docContent3 = Content(docname3, docdata3)

  val docname4 = node4Config.prefix.append("doc", "test4")
  val docdata4 = "four four four four".getBytes
  val docContent4 = Content(docname4, docdata4)

  val node1 = Node(node1Config, withLocalAM = false)
  val node2 = Node(node2Config, withLocalAM = false)
  val node3 = Node(node3Config, withLocalAM = false)
  val node4 = Node(node4Config, withLocalAM = false)//, withComputeServer = false)

  node1 <~> node2
  node1 <~> node3
  node3 <~> node2
  node2 <~> node4

  node1.addNodeFace(node4, node2)
  node3.addNodeFace(node4, node2)
  node4.addNodeFace(node1, node2)
  node4.addNodeFace(node3, node2)
  node4.addPrefixFace(WordCountService().ccnName, node2)

  node1 += docContent1
  node2 += docContent2
  node3 += docContent3
  node4 += docContent4

  node1.publishServices
  node2.publishServices
  node3.publishServices
  node4.publishServices
//  node4.removeLocalServices
  Thread.sleep(1000)

  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks = false

  val wc = WordCountService().toString
  val ss = SumService().toString
  val ts = Translate().toString

  val exComp: Expr = 10 + 2

  val exThunkVsNoThunk: Expr = (wc call List(docname2)) + (wc call List(docname3))

  val exRouteTowardsData: Expr = wc call List(docname4)

  val exCallCall: Expr = ss call (ts call docname2)


  val exServ1 = wc call List(docname3)
  val exServ2 = wc call List(docname2)
  val exComposedServ: Expr = ss call List(exServ1, exServ2)

  val exAddAdd = ((wc call docname1) + (wc call docname2)) + ((wc call docname3) + (wc call docname4))

  val expr = exCallCall

  import AkkaConfig.timeout

  var startTime = System.currentTimeMillis()
  node1 ? expr onComplete {
    case Success(content) => {
      val totalTime = System.currentTimeMillis - startTime
      println(s"Res(${totalTime}): $content")
    }
    case Failure(error) => throw error
  }

  Thread.sleep(AkkaConfig.timeoutDuration.toMillis + 100)

  Monitor.monitor ! Monitor.Visualize()

  node1.shutdown()
  node2.shutdown()
  node3.shutdown()
  node4.shutdown()
}

