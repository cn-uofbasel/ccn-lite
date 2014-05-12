package evaluation

import scala.util._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.pattern._
import akka.util.Timeout

import nfn._
import ccn.packet._
import scala.concurrent.Future
import node.Node
import monitor.Monitor
import lambdacalculus.parser.ast.{Variable, Expr, LambdaDSL}
import nfn.service.impl.{WordCountService, Publish}


object ThreeNodeTwoHopEnv extends App {

  val timeoutDuration: FiniteDuration = 8 seconds
  implicit val timeout = Timeout( timeoutDuration)

  val node1Config = NodeConfig("127.0.0.1", 10010, 10011, CCNName("node", "node1"))
  val node2Config = NodeConfig("127.0.0.1", 10020, 10021, CCNName("node", "node2"))
  val node3Config = NodeConfig("127.0.0.1", 10030, 10031, CCNName("node", "node3"))

  val docname1 = node1Config.prefix.append("doc", "test1")
  val docdata1 = "one".getBytes
  val docContent1 = Content(docname1, docdata1)


  val docname2 = node2Config.prefix.append("doc", "test2")
  val docdata2 = "two two".getBytes
  val docContent2 = Content(docname2, docdata2)

  val docname3 = node3Config.prefix.append("doc", "test3")
  val docdata3 = "three three three".getBytes
  val docContent3 = Content(docname3, docdata3)

  val doc3PrefixToAddContent = Content(node3Config.prefix.append("prefixToAdd", "doc", "test3"), "/dynadded/doc/test3".getBytes)

  var node1 = Node(node1Config)
  var node2 = Node(node2Config)
  var node3 = Node(node3Config)

  Node.connectLine(Seq(node1, node2, node3))
  node1.addFace(node3, node2)
  node3.addFace(node1, node2)

  node1 += docContent1
  node1 += Content(CCNName("node", "node1"), "".getBytes)
  node2 += docContent2
  node3 += docContent3 // /node/node3/doc/test3
//  node3 += Content(CCNName("node", "node3", "doc", "test3", "derp"), "derp".getBytes) // /node/node3/doc/test3/derp
  node3 += doc3PrefixToAddContent

  Thread.sleep(200)

  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks = true

  val pub = Publish().toString

  val wc = WordCountService().toString

  val compExpr: Expr = 'x @: ("y" @: (('x * 1) + "y")  ! 2) ! 3

  val wcExpr: Expr = wc call(List(docname1, docname2, docname3))

  val pubExpr: Expr = pub call(List(docname3, doc3PrefixToAddContent.name, CCNName("node", "node1")))
//
//  node1 ? pubExpr onComplete {
//    case Success(content) => println(s"ACK ADDED: $content")
//    case Failure(error) => throw error
//  }

  Thread.sleep(200)

  val expr = wcExpr
  node1 ? expr onComplete {
    case Success(content) => println(s"PUBLISHED: $content")
    case Failure(error) => throw error
  }

//  Thread.sleep(1000)
//
//  node1 ? Interest(CCNName("node", "node1", "dynadded", "doc", "test3")) onComplete {
//    case Success(content) => println(s"RECVRESULT: $content")
//    case Failure(error) => throw error
//  }

  Thread.sleep(timeoutDuration.toMillis + 100)

  Monitor.monitor ! Monitor.Visualize()

  node1.shutdown
  node2.shutdown
  node3.shutdown
}

