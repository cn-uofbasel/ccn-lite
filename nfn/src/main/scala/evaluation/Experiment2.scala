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
import nfn.service._
import akka.actor.ActorRef
import nfn.service.impl.WordCountService
import nfn.service.impl.SumService
import scala.util.Failure
import scala.Some
import nfn.service.NFNServiceArgumentException
import lambdacalculus.parser.ast.Call
import nfn.CombinedNodeConfig
import nfn.service.impl.Translate
import scala.util.Success
import nfn.ComputeNodeConfig
import nfn.service.NFNBinaryDataValue
import nfn.NFNNodeConfig

case class TestService() extends NFNService {
  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    if(args.size == 0) Try(args)
    else throw argumentException(args)
  }

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (docs, _) =>
    println("yay")
    NFNEmptyValue()
  }

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$ccnName does not take any arguments")
}

object Experiment2 extends App {

  val node1Prefix = CCNName("node", "node1")
  val node1Config = CombinedNodeConfig(Some(NFNNodeConfig("127.0.0.1", 10010, node1Prefix)), Some(ComputeNodeConfig("127.0.0.1", 10011, node1Prefix)))
  val node2Prefix = CCNName("node", "node2")
  val node2Config = CombinedNodeConfig(Some(NFNNodeConfig("127.0.0.1", 10020, node2Prefix)), Some(ComputeNodeConfig("127.0.0.1", 10021, node2Prefix)))
  val node3Prefix = CCNName("node", "node3")
  val node3Config = CombinedNodeConfig(Some(NFNNodeConfig("127.0.0.1", 10030, node3Prefix)), Some(ComputeNodeConfig("127.0.0.1", 10031, node3Prefix)))
  val node4Prefix = CCNName("node", "node4")
  val node4Config = CombinedNodeConfig(Some(NFNNodeConfig("127.0.0.1", 10040, node4Prefix)), Some(ComputeNodeConfig("127.0.0.1", 10041, node4Prefix)))

  val docname1 = node1Prefix.append("doc", "test1")
  val docdata1 = "one".getBytes
  val docContent1 = Content(docname1, docdata1)

  val docname2 = node2Prefix.append("doc", "test2")
  val docdata2 = "two two".getBytes
  val docContent2 = Content(docname2, docdata2)

  val docname3 = node3Prefix.append("doc", "test3")
  val docdata3 = "three three three".getBytes
  val docContent3 = Content(docname3, docdata3)

  val docname4 = node4Prefix.append("doc", "test4")
  val docdata4 = "four four four four".getBytes
  val docContent4 = Content(docname4, docdata4)

  val node1 = Node(node1Config)
  val node2 = Node(node2Config)
  val node3 = Node(node3Config)
  val node4 = Node(node4Config)

  node1 <~> node2
  node1 <~> node3
  node3 <~> node2
  node2 <~> node4

  node1.addNodeFace(node4, node2)
  node3.addNodeFace(node4, node2)
  node4.addNodeFace(node1, node2)
  node4.addNodeFace(node3, node2)

  node4.addPrefixFace(WordCountService().ccnName, node2)

  node1.addPrefixFace(WordCountService().ccnName, node2)
  node1.addPrefixFace(Translate().ccnName, node2)

  node1 += docContent1
  node2 += docContent2
  node3 += docContent3
  node4 += docContent4

//  node1.publishServices
  node2.publishServices
  node3.publishServices
//  node4.publishServices

//  node1.removeLocalServices
  node4.removeLocalServices
//  node2.removeLocalServices
//  node3.removeLocalServices
//  node4.removeLocalServices

  node2.publishService(WordCountService())

  Thread.sleep(1000)

  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks: Boolean = false

  val wc = WordCountService().toString
  val ss = SumService().toString
  val ts = Translate().toString

  val exComp: Expr = 'x @: ('x + 10) ! 2

  val exSimpleCall: Expr = wc appl docname2

  val exThunkVsNoThunk: Expr = (wc appl List(docname2)) + (wc appl List(docname3))

  val exRouteTowardsData: Expr = wc appl List(docname4)

  val exCallCall: Expr = wc appl (ts appl docname2)

  val exCallCallCall: Expr = wc appl (ts appl (ts appl docname2))


  val exServ1 = wc appl List(docname3)
  val exServ2 = wc appl List(docname2)
  val exComposedServ: Expr = ss appl List(exServ1, exServ2)

  val exAddAdd = ((wc appl docname1) + (wc appl docname2)) + ((wc appl docname3) + (wc appl docname4))

  val exIf = iif((wc appl docname2) === 2, ts appl docname2, ts appl docname3)

  val exprString: String = "x" @: Call("x", Variable(docname2.toString) :: Nil) ! WordCountService().toString
//  val exRouteToService = NFNInterest(WordCountService().ccnName.append(exprString).cmps:_*)
  val exRouteToService = NFNInterest(exprString)


  val expr = exThunkVsNoThunk

  import AkkaConfig.timeout
  var startTime = System.currentTimeMillis()
  node1 ? Interest(docname2) onComplete {
    case Success(content) => {
      val totalTime = System.currentTimeMillis - startTime
      println(s"Res($totalTime): $content")
      Monitor.monitor ! Monitor.Visualize()
    }
    case Failure(error) =>
      Monitor.monitor ! Monitor.Visualize()
      throw error
  }

  Thread.sleep(AkkaConfig.timeoutDuration.toMillis + 100)

  node1.shutdown()
  node2.shutdown()
  node3.shutdown()
  node4.shutdown()
}

