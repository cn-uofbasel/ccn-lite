package secmachine


import secmachine.parser.LambdaParser
import secmachine.compiler.{CBVCompiler, CBNCompiler, Compiler}
import secmachine.machine.CallByName._
import secmachine.machine.CallByValue._
import secmachine.machine._
import secmachine.parser.ast.{LambdaPrettyPrinter, Expr}
import scala.util.{Failure, Success, Try}
import com.typesafe.scalalogging.slf4j.Logging
import secmachine.ExecutionOrder.ExecutionOrder


object ExecutionOrder extends Enumeration {
  type ExecutionOrder = Value

  val CallByName = Value("CallByName")
  val CallByValue = Value("CallByValue")
}

case class ParseException(msg: String) extends Throwable(msg)

object LambdaCalculus extends App {

//  val l = "(\\x.x ADD 1) 2"
//  val l = "1 ADD 2"
  val l ="let mySucc = (\\x. x ADD 1) endlet mySucc 1"
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

case class LambdaCalculus(execOrder: ExecutionOrder.ExecutionOrder, debug: Boolean = false, storeIntermediateSteps: Boolean = false) extends Logging {

  val parser = new LambdaParser()
  val compiler = compilerForExecOrder(debug, execOrder)
  val machine = machineForExecOrder(storeIntermediateSteps, execOrder)


  private def compilerForExecOrder(debug: Boolean, execOrder: ExecutionOrder): Compiler = execOrder match {
    case ExecutionOrder.CallByName => CBNCompiler(debug)
    case ExecutionOrder.CallByValue => CBVCompiler(debug)
  }
  private def machineForExecOrder(storeIntermediateSteps: Boolean, execOrder: ExecutionOrder): Machine = execOrder match {
    case ExecutionOrder.CallByName => CBNMachine(storeIntermediateSteps)
    case ExecutionOrder.CallByValue => CBVMachine(storeIntermediateSteps)
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

    val intermediateStepCount = machine.intermediateConfigurations.fold("-")(_.size.toString)
//    println(s"$code -$intermediateStepCount-> $result")
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

  def execute(code: List[Instruction]): Try[List[Value]] = Try(machine(code))
}
