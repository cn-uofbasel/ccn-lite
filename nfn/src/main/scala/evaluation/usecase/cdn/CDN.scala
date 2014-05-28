package evaluation.usecase.cdn

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
import scala.util.Failure
import nfn.service.NFNServiceArgumentException
import scala.util.Success
import nfn.service.NFNBinaryDataValue
import nfn.NodeConfig

case class ESIInclude() extends NFNService {
  def argumentException(args: Seq[NFNValue]):NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$ccnName requires to arguments of type: name of webpage, name of tag to replace, name of content to replace tag with and not $args")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(v1: NFNBinaryDataValue, v2: NFNBinaryDataValue, v3: NFNBinaryDataValue) => Try(args)
      case _ => throw argumentException(args)
    }
  }

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (values: Seq[NFNValue], _) =>
    values match {
      case Seq(xmlDoc: NFNBinaryDataValue, tagToReplace: NFNBinaryDataValue, contentToReplaceTagWith: NFNBinaryDataValue) => {
        val doc = new String(xmlDoc.data)
        val tag = new String(tagToReplace.data)
        val replaceWith = new String(contentToReplaceTagWith.data)
        val res = doc.replaceAllLiterally(tag, replaceWith)
        NFNStringValue(res)
      }
      case _ => throw argumentException(values)
    }
  }
}

case class RandomAd() extends NFNService {
  def argumentException(args: Seq[NFNValue]):NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$ccnName requires no arguments and not $args")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq() => Try(args)
      case _ => throw argumentException(args)
    }
  }

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (values: Seq[NFNValue], _) =>
    values match {
      case Seq() => {
        val randomAd = s"""<div class="ad">randomly chosen ad at: ${System.currentTimeMillis}</div>"""
        NFNStringValue(randomAd)
      }
      case _ => throw argumentException(values)
    }
  }
}

object CDN extends App {

  val node1Prefix = CCNName("node", "node1")
  val node1Config = CombinedNodeConfig(Some(NFNNodeConfig("127.0.0.1", 10010, node1Prefix)), Some(ComputeNodeConfig("127.0.0.1", 10011, node1Prefix)))
  val node2Prefix = CCNName("node", "node2")
  val node2Config = CombinedNodeConfig(Some(NFNNodeConfig("127.0.0.1", 10020, node2Prefix)), Some(ComputeNodeConfig("127.0.0.1", 10021, node2Prefix)))

  val esiTagname = node1Prefix.append("esi", "tag", "include", "randomad")
  val esiTag = "<esi:include:randomad/>"
  val esiTagData = esiTag.getBytes
  val esiTagContent = Content(esiTagname, esiTagData)

  val webpagename = node1Prefix.append("webpage", "test1")
  val webpagedata =
    s"""
      |<html>
      | <body>
      |   <div>
      |     <h1>html page</h1>
      |     <p>random content</p>
      |     $esiTag
      |   </div>
      | </body>
      |</html>
    """.stripMargin.getBytes
  val webpageContent = Content(webpagename, webpagedata)


  val node1 = Node(node1Config)
  val node2 = Node(node2Config)

  node1 <~> node2

  node1 += webpageContent
  node1 += esiTagContent

  node1.publishService(ESIInclude())

  node1.publishService(RandomAd())
  node2.publishService(RandomAd())

  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks = false

  val esiInclude = ESIInclude().toString
  val randomAd = RandomAd().toString

  val exIncludeAd: Expr = esiInclude appl (webpagename, esiTagname, randomAd appl())

  val expr = exIncludeAd

  import AkkaConfig.timeout

  var startTime = System.currentTimeMillis()
  node1 ? expr onComplete {
    case Success(content) => {
      val totalTime = System.currentTimeMillis - startTime
      println(s"Res($totalTime): ${new String(content.data)}")
    }
    case Failure(error) => throw error
  }

  Thread.sleep(AkkaConfig.timeoutDuration.toMillis + 100)

  Monitor.monitor ! Monitor.Visualize()

  node1.shutdown()
  node2.shutdown()
}

