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

object WordCountEnv extends App {

  val timeoutDuration: FiniteDuration = 5 seconds
  implicit val timeout = Timeout( timeoutDuration)

  val n = 8

  val nodeConfigs=
    (0 until n) map { n =>
      val port = 10000 + n*10
      val computePort = port + 1
      NodeConfig(host = "127.0.0.1", port = port, computePort = computePort, prefix = s"docRepo$n")
    }

  val nodes = nodeConfigs map { Node(_) }

  Node.connectGrid(nodes)

  val docNamesWithoutPrefix =
    (0 until 10) map { n => Seq("docs", s"doc$n") }

  val docContents: List[Array[Byte]] =
    ((0 until 10) map { n => (s"$n "*n).trim.getBytes }).toList

  val indexForDocsOnNodes = List(Nil, List(0, 5, 7), List(2,6,9), List(3,1), List(4,8))// ++ (0 until (n - 5)) map { _ => Nil}

  val nodeNameDatas: List[(Int, Seq[String], Array[Byte])] =
    indexForDocsOnNodes.take(n).zipWithIndex flatMap { case (docIndizes: List[Int], nodeIndex: Int) =>
       docIndizes map { docIndex =>
         val prefix = nodes(nodeIndex).nodeConfig.prefix
         (nodeIndex, Seq(prefix) ++ docNamesWithoutPrefix(docIndex), docContents(docIndex))
      }
    }

  val docNames = nodeNameDatas map { _._2.mkString("/", "/", "")}

  nodeNameDatas foreach { case (nodeId, name, data) =>
    nodes(nodeId) += Content(name, data)
  }

  nodes foreach { _.publishServices }


//  val (reqNodeIndex, nameDoc4Node4, _) = nodeNameDatas.find(_._1 == 4).head
//  val docsNode4: List[(Int, Seq[String], Array[Byte])] = nodeNameDatas.filter(_._1 == 4)
//  val nameDoc8Node4 = docsNode4.filter(_._2.last == "doc8").head._2
//  val reqNode = nodes(reqNodeIndex)

  val map = MapService().nfnName.toString
  val wc = WordCountService().nfnName.toString
  val red = ReduceService().nfnName.toString
  val sum = SumService().nfnName.toString

  val add = AddService().toString

//  (reqNode ? Interest(Seq(s"call ${docNames.size + 2} $map $wc ${docNames.mkString(" ")}", "NFN"))) onComplete {
//  (reqNode ? Interest(Seq(s"call 3 $add 3 4", "NFN"))) onComplete {

//  (reqNode ? Interest(nameDoc2Node2)) onComplete {
//  (nodes.head ? Interest(Seq(s"call 3 $wc ${nameDoc4Node4.mkString("/", "/", "")} ${nameDoc8Node4.mkString("/", "/", "")}", "NFN"))) onComplete {


  def namesToAddedWordCount(docs: Seq[String]): String = {
    def wcDoc(doc: String) =  s"call 2 $wc $doc"

    def add(docs: Seq[String], cur: String = ""): String = {
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
  val expr = namesToAddedWordCount(docNames)
//  val expr = s"call ${docNames.size + 1} $wc ${docNames.mkString(" ")}"
//  (nodes.head ! Interest(Seq(expr, "THUNK", "NFN")))
//  Thread.sleep(800)
//  (reqNode ? Interest(Seq(nameDoc2Node2.mkString("/", "/", ""), "NFN"))) onComplete {
//  (nodes(0) ? Interest(Seq("sub 2 3", "NFN"))) onComplete {
  (nodes(0) ? Interest(Seq(expr, "NFN"))) onComplete {
    case Success(content) => println(s"RESULT: $content")
    case Failure(e) => throw e
  }

  Thread.sleep(timeoutDuration.toMillis)
  Monitor.monitor ! Monitor.Visualize()
  Thread.sleep(200)
  nodes foreach { _.shutdown }
//  sys.exit
}

