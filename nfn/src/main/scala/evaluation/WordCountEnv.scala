package evaluation

import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import scala.util._

import akka.util.Timeout

import ccn.packet._
import node.Node
import nfn.service.impl._
import nfn.NodeConfig

import monitor.Monitor
import akka.pattern.AskTimeoutException

object WordCountEnv extends App {

  val timeoutDuration: FiniteDuration = 5 seconds
  implicit val timeout = Timeout( timeoutDuration)

  val n = 5

  val nodeConfigs=
    (0 until n) map { n =>
      val port = 10000 + n*10
      val computePort = port + 1
      NodeConfig(host = "127.0.0.1", port = port, computePort = computePort, prefix = s"docRepo$n")
    }

  val nodes = nodeConfigs map { Node(_) }

  Node.connectAll(nodes)

  val docNamesWithoutPrefix =
    (0 until 10) map { n => CCNName("docs", s"doc$n") }

  val docContents: List[Array[Byte]] =
    ((0 until 10) map { n => (s"$n "*n).trim.getBytes }).toList

  val indexForDocsOnNodes = List(Nil, List(0, 5, 7), List(2,6,9), List(3,1), List(4,8))// ++ (0 until (n - 5)) map { _ => Nil}

  val nodeNameDatas: List[(Int, CCNName, Array[Byte])] =
    indexForDocsOnNodes.zipWithIndex flatMap { case (docIndizes: List[Int], nodeIndex: Int) =>
       docIndizes map { docIndex =>
         val prefix: String = nodes(nodeIndex).nodeConfig.prefix
         val cmps: Seq[String] = Seq(prefix) ++ docNamesWithoutPrefix(docIndex).cmps
         (nodeIndex, CCNName(cmps:_*), docContents(docIndex))
      }
    }

  val docNamesNode1: List[CCNName] = nodeNameDatas.filter(_._1 == 1).map {
    _._2
  }

  val docNames = nodeNameDatas map { _._2 }

  nodeNameDatas foreach { case (nodeId, name, data) =>
    nodes(nodeId) += Content(name, data)
  }

  nodes foreach { _.publishServices }

  nodes foreach { _.removeLocalServices }

  Thread.sleep(1000)


  val map = MapService().ccnName.toString
  val wc = WordCountService().ccnName.toString
  val red = ReduceService().ccnName.toString
  val sum = SumService().ccnName.toString

  val add = AddService().toString

  def namesToAddedWordCount(docs: List[CCNName]): String = {
    def wcDoc(doc: CCNName) =  s"call 2 $wc $doc"

    def add(docs: List[CCNName], cur: String = ""): String = {
      docs match {
        case Seq() => cur
        case _ =>
          val l = wcDoc(docs.head)
          val next = if(cur == "") {
            l
          } else {
            s"add ($l) ($cur)"
          }
          add(docs.tail, next)
      }
    }
    add(docs)
  }

  //*******************************************************************************

  def printInterestResult(interest: Interest) = {

    Thread.sleep(1000)

    (nodes(0) ? interest) onComplete {
      case Success(content) => println(s"RESULT: $content")
      case Failure(e) => e match {
        case _: AskTimeoutException => println(s"TIMEOUT: $interest")
        case _  => throw e
      }
    }
  }
//
//  val addAllDocsSeperatelyExpression = namesToAddedWordCount(docNames.take(3))
//  val interest = Interest(CCNName(addAllDocsSeperatelyExpression, "NFN"))
//  printInterestResult(interest)



  val addAllDocsNode1 = s"call ${docNamesNode1.size + 1} $wc ${docNamesNode1.mkString(" ")}"
  printInterestResult(Interest(CCNName(addAllDocsNode1, "NFN")))


  val documents: Set[String] = Set("doc1"*1, "doc2"*2, "doc3"*3)

  val wordCount: Int = documents.map {
    (doc: String) => doc.split(" ").length
  } reduce {
    (left: Int, right: Int) => left + right
  }


  //  import lambdamacros.LambdaMacros._
//  (nodes(0) ? Interest(CCNName(lambda({
//    val a = 1
//    6 * (a + 1)
//  }),"NFN"))) onComplete {
//    case Success(content) => println(s"RESULT: $content")
//    case Failure(e) => throw e
//  }

  Thread.sleep(timeoutDuration.toMillis)
  Monitor.monitor ! Monitor.Visualize()
  Thread.sleep(200)
  nodes foreach { _.shutdown }

//  sys.exit
}

