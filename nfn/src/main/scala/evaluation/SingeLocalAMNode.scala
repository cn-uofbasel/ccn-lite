package evaluation

import scala.concurrent.duration._
import akka.util.Timeout
import nfn.{LambdaNFNImplicits, NodeConfig}
import ccn.packet.{Content, CCNName}
import node.Node
import lambdacalculus.parser.ast.{Expr, LambdaDSL}
import scala.util.{Failure, Success}
import scala.concurrent.ExecutionContext.Implicits.global
import nfn.service.impl.{Translate, WordCountService}

/**
 * Created by basil on 14/05/14.
 */
object SingeLocalAMNode extends App {

  val timeoutDuration: FiniteDuration = 8.seconds
  implicit val timeout = Timeout( timeoutDuration)
  import LambdaDSL._
  import LambdaNFNImplicits._
  implicit val useThunks = false

  val nodeConfig = NodeConfig("127.0.0.1", 10010, 10011, CCNName("node", "node1"))
  val node = Node(nodeConfig, withLocalAM = true)

  val docName = CCNName("node", "node1", "doc", "name")
  val content = Content(docName, "test content yo".getBytes)

  node cache content

  val wc = WordCountService().toString

  val translate = Translate().toString

  val wcExpr: Expr = wc appl List(docName)
  val wcTranslateExpr: Expr = wc appl List(translate appl List(docName))

  val compExpr: Expr = 'x @: ("y" @: (('x * 1) + "y")  ! 2) ! 3

  val expr = wcTranslateExpr
  (node ?  wcExpr) onComplete {
    case Success(content) => println(s"RESULT: $content")
    case Failure(e) => throw e
  }
}
