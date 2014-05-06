package evaluation.usecase

import nfn.NodeConfig
import node.Node
import ccn.packet._
import scala.util.{Failure, Success}
import scala.concurrent.ExecutionContext.Implicits.global
import akka.pattern.AskTimeoutException
import scala.concurrent.duration._
import akka.util.Timeout
import lambdacalculus.LambdaCalculus
import lambdacalculus.parser.ast.LambdaNFNPrinter

object LambdaExpressionTester extends App {

  val timeoutDuration: FiniteDuration = 5 seconds
  implicit val timeout = Timeout( timeoutDuration)

  val nodeConfig = NodeConfig("localhost", 10010, 10011, CCNName("node1"))

  val node = Node(nodeConfig)

  Thread.sleep(1000)

  val rawExpr =
  """
    |let true = 1 endlet
    |let false = 0 endlet
    |let pair = λa.λb.λf.(f a b) endlet
    |let first  = λp.p (λa.λb.a) endlet
    |let second = λp.p (λa.λb.b) endlet
    |let empty = λf.λx.x endlet
    |let append = λa.λl.λf.λx.f a (l f x) endlet
    |let head = λl.l(λa.λb.a) dummy endlet
    |let isempty = λl.l (λa.λb.false) true endlet
    |let map = λl.λh.λf.λx.l ((λm.f m) h x) endlet
    |let tail = λl.first (l (λa.λb.pair (second b) (append a (second b)) ) (pair empty empty)) endlet
    |head (map  (append 4 ( append 3 (append 2 empty))) (λa.add a 1))
  """.stripMargin

  LambdaCalculus().substituteParse(rawExpr).map(expr => {
    val exprNFNString = LambdaNFNPrinter(expr).replace("\n", "")
    println(s"Sending $exprNFNString to network")
    val interest: Interest = Interest(exprNFNString, "NFN")

    (node ? interest) onComplete {
      case Success(content) => println(s"RESULT: $content")
      case Failure(e) => e match {
        case _: AskTimeoutException => println(s"TIMEOUT: $interest")
        case _ => println("expection while asking"); throw e
      }
    }
  }).failed

  Thread.sleep(timeoutDuration.toMillis + 100)
  node.shutdown()
//  sys.exit
}
