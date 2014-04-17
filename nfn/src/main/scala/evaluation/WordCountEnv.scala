package evaluation

import scala.concurrent.duration.FiniteDuration
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global

import akka.util.Timeout
import nfn.NodeConfig
import node.Node
import ccn.packet._
import scala.util._
import nfn.service.impl._
import nfn.service.NFNServiceLibrary
import ccn.packet.Interest
import nfn.service.impl.MapService
import nfn.service.impl.WordCountService
import nfn.service.impl.SumService
import nfn.service.impl.ReduceService
import scala.util.Failure
import scala.util.Success
import ccn.packet.Content
import nfn.NodeConfig

object WordCountEnv extends App {

  val timeoutDuration: FiniteDuration = 6 seconds
  implicit val timeout = Timeout( timeoutDuration)



  val nodeConfigs=
    (0 until 5) map { n =>
      val port = 10000 + n*10
      val computePort = port + 1
      NodeConfig("localhost", port, computePort, s"docRepo$n")
    }

  val nodes = nodeConfigs map { Node(_) }

  Node.connectAll(nodes)


  val docNamesWithoutPrefix =
    (0 until 10) map { n => Seq("docs", s"doc$n") }

  val docContents: List[Array[Byte]] =
    ((0 until 10) map { n => (s"$n "*n).trim.getBytes }).toList

  val indexForDocsOnNodes = List(Nil, List(0, 5, 7), List(2,6,9), List(3,1), List(4,8))

  val nodeNameDatas: List[(Int, Seq[String], Array[Byte])] =
    indexForDocsOnNodes.zipWithIndex flatMap { case (docIndizes: List[Int], nodeIndex: Int) =>
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


  val (reqNodeIndex, nameDoc4Node4, _) = nodeNameDatas.find(_._1 == 4).head
  val docsNode4: List[(Int, Seq[String], Array[Byte])] = nodeNameDatas.filter(_._1 == 4)
  val nameDoc8Node4 = docsNode4.filter(_._2.last == "doc8").head._2
  val reqNode = nodes(reqNodeIndex)

  val map = MapService().nfnName.toString
  val wc = WordCountService().nfnName.toString
  val red = ReduceService().nfnName.toString
  val sum = SumService().nfnName.toString

  val add = AddService().toString

//  (reqNode ? Interest(Seq(s"call ${docNames.size + 2} $map $wc ${docNames.mkString(" ")}", "NFN"))) onComplete {
//    (reqNode ? Interest(Seq(nameDoc2Node2.mkString("/", "/", ""), "NFN"))) onComplete {
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
  (nodes(1) ? Interest(Seq(expr, "NFN"))) onComplete {
    case Success(content) => println(s"RESULT: $content")
    case Failure(e) => throw e
  }

  Thread.sleep(7000)
  nodes foreach { _.shutdown }

  sys.exit
}

