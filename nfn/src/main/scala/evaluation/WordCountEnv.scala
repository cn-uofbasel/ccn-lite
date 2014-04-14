package evaluation

import scala.concurrent.duration.FiniteDuration
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global

import akka.util.Timeout
import nfn.NodeConfig
import node.Node
import ccn.packet._
import scala.util._
import nfn.service.impl.{SumService, ReduceService, WordCountService, MapService}

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
    ((0 until 10) map { n => ("content: '" + s"$n"*n + "'").getBytes }).toList

  val indexForDocsOnNodes = List(Nil, List(0, 5, 7), List(2,6,9), List(3,1), List(4,8))

  val nodeNameDatas: List[(Int, Seq[String], Array[Byte])] =
    indexForDocsOnNodes.zipWithIndex flatMap { case (docIndizes: List[Int], nodeIndex: Int) =>
       docIndizes map { docIndex =>
         val prefix = nodes(nodeIndex).nodeConfig.prefix
         (nodeIndex, Seq(prefix) ++ docNamesWithoutPrefix(docIndex), docContents(docIndex))
      }
    }

  println(nodeNameDatas)

  val docNames = nodeNameDatas map { _._2.mkString("/", "/", "")}

  nodeNameDatas foreach { case (nodeId, name, data) =>
    nodes(nodeId) += Content(name, data)
  }


  val (reqNodeIndex, nameDoc2Node2, _) = nodeNameDatas.find(_._1 == 3).head
  val reqNode = nodes(0)//(reqNodeIndex)
  Thread.sleep(1000)

//  (reqNode ? Interest(nameDoc0Node1)) onComplete {
//    case Success(content) => println(s"RCV: $content")
//    case Failure(e) => throw e
//  }

  val map = MapService().nfnName.toString
  val wc = WordCountService().nfnName.toString
  val red = ReduceService().nfnName.toString
  val sum = SumService().nfnName.toString

//  (reqNode ? Interest(Seq(s"call ${docNames.size + 2} $map $wc ${docNames.mkString(" ")}", "NFN"))) onComplete {
//  (reqNode ? Interest(Seq(s"call 2 $wc $nameDoc2Node2", "NFN"))) onComplete {
    (reqNode ? Interest(nameDoc2Node2)) onComplete {
    case Success(content) => println(s"RESULT: $content")
    case Failure(e) => throw e
  }

  Thread.sleep(10000)
  nodes foreach { _.shutdown }

  sys.exit
}

