package lambdacalculus

import lambdacalculus.machine._
import lambdacalculus.ExecutionOrder.ExecutionOrder
import scala.util.{Failure, Success}


object REPLState {
  private var _debug = true

  private var _storeIntermediateResults = true

  private var _execOrder = ExecutionOrder.CallByValue

  private var _lambdaCalculus = init()

  private def init(): LambdaCalculus = {
    _lambdaCalculus =  LambdaCalculus(_execOrder, _debug, _storeIntermediateResults)
    _lambdaCalculus
  }

  def storeIntermediateResults(b: Boolean) = {
    _storeIntermediateResults = b
    init()
  }
  def debug(b: Boolean) = {
    _debug = b
    init()
  }

  def execOrder(o: ExecutionOrder) = {
    _execOrder = o
    init()
  }

  def lambdaCalculus: LambdaCalculus = _lambdaCalculus
}

object REPL {

  def main(args: Array[String]) {

    if(args.size == 1) {
      REPLState.lambdaCalculus.substituteParseCompileExecute(args(0)) match {
        case Success(res) => println(res)
        case Failure(error) => {
          error.printStackTrace(System.err)
          System.exit(1)
        }
      }
    } else if(args.size == 0) {
      while(true) {
        val input = readLine(consolePrefix(inputPrefix))
        if (input.startsWith(commandPrefix))
          handleCmd(input.substring(1).toLowerCase())
        //      else if(input contains "=")
        //        handleDef(input)
        else
          handleExpr(input)
      }
    } else {
      println("Run with 0 parameters for REPL and 1 parameter to execute string as lambda expr")
    }
  }

  def handleCmd(cmd: String) =
    cmd.split(" ") match {
      case Array("quit" | "exit" | "q" | "e") => System.exit(0)
      case Array("debug" | "d", arg) => arg match {
          case "on" | "yes" | "true" => REPLState.storeIntermediateResults(true)
          case "off" | "no" | "false" => REPLState.storeIntermediateResults(false)
      }
      case Array("machine" | "m", arg) => arg match {
        case "cbn" => REPLState.execOrder(ExecutionOrder.CallByName)
        case "cbv" => REPLState.execOrder(ExecutionOrder.CallByValue)
      }

      case Array("help" | "h") =>
        println(
          """
            |Lambda Evalautor         - type in lambda expressions like \x.\y.y x, mysucc = λn.add 1 n  or add 2
            |- help | h               - prints help
            |- machine | m <cbn/cbv>  - switches execution order of the abstract machine
            |- debug | d <on/off>     - prints all intermediate configuration of the machine
            |- quit | exit | q | e    - exit
          """.stripMargin)
      case _ => println("Unkown command: " + cmd )

    }

  def handleExpr(input: String, stepByStep: Boolean = false) = {
    val lambdaCalculus = REPLState.lambdaCalculus

      lambdaCalculus.substituteParseCompileExecute(input) match {
        case Success(resultStack: List[Value]) => {
          // if(lambdaCalculus.machine.storeIntermediateSteps) machine.printIntermediateSteps()
          println(s"resultstack: $resultStack")
          val strResult: List[String] = resultStack.map(ValuePrettyPrinter(_, Some(lambdaCalculus.compiler)))
          println(consolePrefix(resultPrefix) + strResult.mkString(",")) //resultStack.map(ValuePrettyPrinter(_, Some(lambdaCalculus.compiler))).mkString(","))
        }
        case Failure(error) => {
          System.err.println(errorPrefix + error)
        }
      }
  }

  def inputPrefix = ">"
  def resultPrefix = "="
  def errorPrefix = "!"
  def commandPrefix = ":"

  def consolePrefix(prefixType: String) = s"λ$prefixType "
}
