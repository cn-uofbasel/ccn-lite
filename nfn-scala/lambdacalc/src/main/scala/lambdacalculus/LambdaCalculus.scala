package lambdacalculus


import lambdacalculus.parser.{LambdaParser, StandardLambdaParser}
import lambdacalculus.compiler.{CBVCompiler, CBNCompiler, Compiler}
import lambdacalculus.machine.CallByName._
import lambdacalculus.machine.CallByValue._
import lambdacalculus.machine._
import lambdacalculus.parser.ast.Expr
import scala.util.{Failure, Success, Try}
import com.typesafe.scalalogging.slf4j.Logging
import lambdacalculus.ExecutionOrder.ExecutionOrder


object ExecutionOrder extends Enumeration {
  type ExecutionOrder = Value

  val CallByName = Value("CallByName")
  val CallByValue = Value("CallByValue")
}

case class ParseException(msg: String) extends Throwable(msg)

object LambdaCalculusDecompilation extends App {

//  val l = "(\\x.x ADD 1) 2"
//  val l = "1 ADD 2"
  val l ="let mySucc = (\\x. add x 1) endlet mySucc 1"
  val lc = LambdaCalculus(ExecutionOrder.CallByValue, true, false)
  val parsed =
    for {s <- lc.substituteLibrary(l)
         p <- lc.parse(s)}
    yield p
  val compiled = parsed.flatMap(lc.compile)

  val decompiled = compiled match {
    case Success(c) => {
      lc.compiler.decompile(c)
    }
    case Failure(e) => throw new Exception(s"Could not decompile $e")
  }
  println(parsed.get)
  println("====================================")
  println(compiled.get)
  println("====================================")
  println(decompiled)

}

object LambdaCalculus extends App {
  val l =
    """
      |call 3 SumService 11 1
    """.stripMargin
//    """
//      | let countToTen =
//      |   \x.
//      |     if (x LT 10)
//      |     then countToTen (x ADD 1)
//      |     else x
//      |endlet
//      |
//      |countToTen 9
//    """.stripMargin
  val lc = LambdaCalculus(ExecutionOrder.CallByValue)
  lc.substituteParseCompileExecute(l)
}

case class LambdaCalculus(execOrder: ExecutionOrder.ExecutionOrder = ExecutionOrder.CallByValue,
                          debug: Boolean = false,
                          storeIntermediateSteps: Boolean = false,
                          maybeExecutor: Option[CallExecutor] = None,
                          parser: LambdaParser = new StandardLambdaParser()) extends Logging {

  val compiler = compilerForExecOrder(debug, execOrder)
  val machine = machineForExecOrder(storeIntermediateSteps, execOrder, maybeExecutor)


  private def compilerForExecOrder(debug: Boolean, execOrder: ExecutionOrder): Compiler = execOrder match {
    case ExecutionOrder.CallByName => CBNCompiler(debug)
    case ExecutionOrder.CallByValue => CBVCompiler(debug)
  }
  private def machineForExecOrder(storeIntermediateSteps: Boolean, execOrder: ExecutionOrder, maybeExecutor: Option[CallExecutor]): AbstractMachine = execOrder match {
    case ExecutionOrder.CallByName => CBNAbstractMachine(storeIntermediateSteps)
    case ExecutionOrder.CallByValue => CBVAbstractMachine(storeIntermediateSteps, maybeExecutor)
  }

  def substituteParse(code: String): Try[Expr] = {
    for {
      substituted <- substituteLibrary(code)
      parsed <- parse(substituted)
    } yield parsed
  }

  def substituteParseCompile(code: String): Try[List[Instruction]] = {
    for {
      parsed <- substituteParse(code)
      compiled <- compile(parsed)
    } yield compiled
  }

  def substituteParseCompileExecute(code: String): Try[List[Value]] = {
    val result = for {
      compiled <- substituteParseCompile(code)
      executed <- execute(compiled)
    } yield executed

    val intermediateStepCount = machine.intermediateConfigurations.fold("?")(_.size.toString)
    logger.info(s"$code -$intermediateStepCount-> $result")

    result
  }

  def substituteLibrary(code: String): Try[String] = Try(CommandLibrary(compiler)(code))

  def parse(code: String):Try[Expr]  = {
    import parser.{ Success, NoSuccess }

    parser(code) match {
      case Success(res: Expr, _) => Try(res)
      case NoSuccess(err, next) => {
        val msg = s"\n$err:\n${next.pos.longString}\n"
        throw new ParseException(msg)
      }
    }
  }

  def compile(code: Expr): Try[List[Instruction]] = Try(compiler(code))

  def execute(code: List[Instruction]): Try[List[Value]] = {
    Try(machine(code))
  }
}
