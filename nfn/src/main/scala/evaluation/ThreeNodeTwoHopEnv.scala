package evaluation

import scala.util._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.{ActorRef, ActorSystem}
import akka.pattern._
import akka.util.Timeout
import config.AkkaConfig

import nfn._
import ccn.packet._
import nfn.NFNMaster._
import scala.concurrent.Future
import node.Node
import monitor.Monitor


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

  var node1 = Node(node1Config)
  var node2 = Node(node2Config)
  var node3 = Node(node3Config)

  Node.connectLine(Seq(node1, node2, node3))
  node1.addFace(node3, node2)
  node3.addFace(node1, node2)

  Thread.sleep(1000)
  node1 += docContent1
  node2 += docContent2
  node3 += docContent3

  Thread.sleep(1000)

  node1 ? Interest(docname3) onComplete {
    case Success(content) => println(s"RECV FROM REMOTE: $content")
    case Failure(error) => throw error
  }

  Thread.sleep(timeoutDuration.toMillis + 100)

  Monitor.monitor ! Monitor.Visualize()

  node1.shutdown
  node2.shutdown
  node3.shutdown
}

