package evaluation

import com.sun.xml.internal.ws.policy.privateutil.PolicyUtils.ConfigFile
import com.typesafe.config.ConfigFactory

import scala.concurrent.duration._
import akka.util.Timeout
import nfn._
import ccn.packet.{Content, CCNName}
import node.LocalNode
import lambdacalculus.parser.ast.{Expr, LambdaDSL}
import scala.util.{Failure, Success}
import scala.concurrent.ExecutionContext.Implicits.global
import nfn.service.impl.{Translate, WordCountService}
import nfn.CombinedNodeConfig
import nfn.service.impl.Translate
import nfn.service.impl.WordCountService
import scala.util.Success
import scala.util.Failure
import scala.Some
import nfn.RouterConfig

/**
 * Created by basil on 14/05/14.
 */
object SingeLocalAMNode extends App {

  val timeoutDuration: FiniteDuration = 8.seconds
  implicit val timeout = Timeout( timeoutDuration)
  implicit val config = ConfigFactory.load()
  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks = false

  val nodePrefix = CCNName("node", "node1")
  val node = LocalNode(
    RouterConfig("127.0.0.1", 10010, nodePrefix),
    Some(ComputeNodeConfig("127.0.0.1", 10011, nodePrefix, withLocalAM = true))
  )


  val docName = CCNName("node", "node1", "doc", "name")
  val content = Content(docName, "test content yo".getBytes)

  node cache content

  val wc = new WordCountService().toString

  val translate = new Translate().toString

  val wcExpr: Expr = wc appl List(docName)
  val wcTranslateExpr: Expr = wc appl List(translate appl List(docName))

  val compExpr: Expr = 'x @: ("y" @: (('x * 1) + "y")  ! 2) ! 3

  val expr = wcTranslateExpr
  (node ?  wcExpr) onComplete {
    case Success(content) => println(s"RESULT: $content")
    case Failure(e) => throw e
  }
}
