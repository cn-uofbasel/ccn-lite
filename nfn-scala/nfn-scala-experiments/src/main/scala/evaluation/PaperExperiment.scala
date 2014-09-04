package evaluation

import akka.actor.{ActorLogging, ActorRef}
import com.typesafe.config.{Config, ConfigFactory}
import nfn.service._

import scala.util._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.util.Timeout

import nfn._
import ccn.packet._
import scala.concurrent.Future
import node.{StandardNodeFactory, LocalNode}
import monitor.Monitor
import lambdacalculus.parser.ast._
import nfn.service.impl._
import config.AkkaConfig
import java.io.File


object PaperExperiment extends App {

  implicit val conf: Config = ConfigFactory.load()

  val expNum = 2

  val node1 = StandardNodeFactory.forId(1)
  val node2 = StandardNodeFactory.forId(2, isCCNOnly = true)

  val node3 = StandardNodeFactory.forId(3)

  val node4 = StandardNodeFactory.forId(4)
  val node5 = StandardNodeFactory.forId(5, isCCNOnly = true)
  val nodes = List(node1, node2, node3, node4, node5)

  val docname1 = node1.prefix.append("doc", "test1")
  val docdata1 = "one".getBytes

  val docname2 = node2.prefix.append("doc", "test2")
  val docdata2 = "two two".getBytes

  val docname3 = node3.prefix.append("doc", "test3")
  val docdata3 = "three three three".getBytes

  val docname4 = node4.prefix.append("doc", "test4")
  val docdata4 = "four four four four".getBytes

  val docname5 = node5.prefix.append("doc", "test5")
  val docdata5 = "five five five five five".getBytes

  node1 <~> node2
  if(expNum != 3) {
    node1.addNodeFaces(List(node4), node2)
    node2.addNodeFaces(List(node3), node1)
  } else {
    node1.addNodeFaces(List(node3, node4, node5), node2)
  }

  if(expNum != 3) {
    node1 <~> node3
    node1.addNodeFaces(List(node4), node3)
  }

  node2 <~> node4
  node2.addNodeFaces(List(node3, node5), node4)
  node4.addNodeFaces(List(node1), node2)

  node3 <~> node4
  node3.addNodeFaces(List(node2), node4)
  node4.addNodeFaces(List(node1), node3)

  node3 <~> node5
  node5.addNodeFaces(List(node1), node3)

  // remove for exp 6
  if(expNum != 6) {
    node4 <~> node5
    node5.addNodeFaces(List(node2), node4)
  } else {
    node4.addNodeFace(node5, node3)
  }
  node1 += Content(docname1, docdata1)
  node2 += Content(docname2, docdata2)
  node3 += Content(docname3, docdata3)
  node4 += Content(docname4, docdata4)
  node5 += Content(docname5, docdata5)

  // remove for exp6
  if(expNum != 6) {
    node3.publishService(new WordCountService())
  }

  node4.publishService(new WordCountService())

  val wcPrefix = new WordCountService().ccnName

  // remove for exp3
  if(expNum != 3 && expNum != 7) {
    node1.addPrefixFace(wcPrefix, node3)
  } else if(expNum != 7) {
    node1.addPrefixFace(wcPrefix, node2)
  }

  node2.addPrefixFace(wcPrefix, node4)
  if(expNum == 7) {
    node2.addPrefixFace(wcPrefix, node1)
  }

  node5.addPrefixFace(wcPrefix, node3)

  if(expNum != 6) {
    node5.addPrefixFace(wcPrefix, node4)
  } else {

    node3.addPrefixFace(wcPrefix, node4)
  }

  val dynServ = new NFNDynamicService {
    override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (_, _) =>
      println("yay")
      NFNIntValue(42)
    }
  }
  node1.publishService(dynServ)

  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks: Boolean = false

  val ts = new Translate().toString
  val wc = new WordCountService().toString
  val nack = new NackServ().toString
  val dyn = dynServ.toString

  val exp1 = wc appl docname1

  val exp2 = wc appl docname5

  // cut 1 <-> 3:
  // remove <~>
  // remove prefixes
  // add wc face to node 2
  // remove wc face to node 3
  val exp3 = wc appl docname5

  // thunks vs no thunks
  val exp4 = (wc appl docname3) + (wc appl docname4)

  val exp5_1 = wc appl docname3
  val exp5_2 = (wc appl docname3) + (wc appl docname4)

  // node 3 to ccn only (simluate "overloaded" router)
  // cut 4 <-> 5
  // wc face from 3 to 4
  val exp6 = wc appl docname5

  // Adds the wordcountservice to node1 and adds routing from node2 to 1
  val exp7 = (wc appl docname4) + (wc appl docname3)

  val exp8 = nack appl

  val exp9 = dyn appl

  expNum match {
    case 1 => doExp(exp1)
    case 2 => doExp(exp2)
    case 3 => doExp(exp3)
    case 4 => doExp(exp4)
    case 5 => doExp(exp5_1); Thread.sleep(2000); doExp(exp5_2)
    case 6 => doExp(exp6)
    case 7 => doExp(exp7)
    case 8 => doExp(exp8)
    case 9 => doExp(exp9)
    case _ => throw new Exception(s"expNum can only be 1 to 7 and not $expNum")
  }

  def doExp(exprToDo: Expr) = {
    import AkkaConfig.timeout
    val startTime = System.currentTimeMillis()
    node1 ? exprToDo andThen {
      case Success(content) => {
        val totalTime = System.currentTimeMillis - startTime
        println(s"RESULT($totalTime): $content")
      }
      case Failure(error) =>
        throw error
//        Monitor.monitor ! Monitor.Visualize()
    }
  }
  Thread.sleep(StaticConfig.defaultTimeoutDuration.toMillis + 100)
  Monitor.monitor ! Monitor.Visualize()
  nodes foreach { _.shutdown() }
}

