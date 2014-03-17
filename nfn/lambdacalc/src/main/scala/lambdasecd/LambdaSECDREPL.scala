package lambdasecd


object LambdaREPL {
  var numberMode = true
  val parser = new LambdaSECDParser()
  val pretty = new PrettyPrinter()
  var machine = new LambdaSECDMachine(true)

  val code = "((λx.(mult x ((λy. sqrt y) 5))) 3)"
//  val code = "(\\x.(mult x 3)) 4"


  def main(args: Array[String]) {
    handleExpr(code)
    while(true) {
      val input = readLine("λ> ")
      if (input.startsWith(":"))
        handleCmd(input.substring(1).toLowerCase())
//      else if(input contains "=")
//        handleDef(input)
      else
        handleExpr(input)
    }
  }

  def handleCmd(cmd: String) =
    cmd.split(" ") match {
//      case Array("step" | "s", exprSrc) =>
//        parseInput(parser.parse, exprSrc) { expr =>
//          try {
//            val in = bind(expr)
//            println(pretty(in) + " →")
//            val out = eval.evalStep(in)
//            println(pretty(out))
//          } catch {
//            case _: MatchError => println("Cannot reduce further.")
//          }
//        }
      case Array("quit" | "exit" | "q" | "e") => System.exit(0)
      case Array("debug" | "d", arg) =>
        arg match {
          case "on" => machine = new LambdaSECDMachine(true)
          case "off" => machine = new LambdaSECDMachine(false)
        }
      case Array("number" | "n", arg) => arg match {
        case "on" | "yes" | "true" => numberMode = true
        case "off" | "no" | "false" => numberMode = false
      }

      case Array("help" | "h") =>
        println(
          """
            |Lambda Evalautor - type in lambda expressions like \x.\y.y x, mysucc = λn.add 1 n  or add 2
            |- quit | exit | q | e - exit
            |- debug (on | off) - enables debug output of the application steps
            |- number | n (on | off) - when on assumes that all results should be numbers
          """.stripMargin)
      case _ => println("Unkown command: " + cmd )

    }

//  def handleDef(input: String) =
//    parseInput(parser.definitions, input) { defs =>
//      println("Defined: " + defs.keys.mkString(", "))
//      bind = new Binder(bind.defs ++ defs)}

  def handleExpr(input: String) =
    parseInput(parser.parse, input) { expr =>
      val S = machine(expr)
      println(S match {
        case Nil => "! No Result"
        case h :: Nil => h
        case _ => S.mkString("(", ", ", ")")
      })
    }

  // A parser and the input as argument and returns a function from type T to Unit if the parser succeeds.
  def parseInput[T](p: String => parser.ParseResult[T], input: String)(success: T => Unit): Unit = {
    import parser.{ Success, NoSuccess }
    p(input) match {
      case Success(res, _) => success(res)
      case NoSuccess(err, _) => println(err)
    }
  }
}
