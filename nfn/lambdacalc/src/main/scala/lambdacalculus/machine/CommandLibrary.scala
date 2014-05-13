package lambdacalculus.machine

import lambdacalculus.parser.StandardLambdaParser
import lambdacalculus.parser.ast.{Let, Expr}
import lambdacalculus.compiler.Compiler

case class CommandInstruction(name: String, instructions: List[Instruction])

case class CommandLibrary(compiler: Compiler) {

//  private val parsedLib:List[Let] = {
//    val parser = new StandardLambdaParser()
//    parser.definitions(lib) match {
//      case parser.Success(defs:List[Let], _) => defs
//      case parser.NoSuccess(msg, input) => throw new Exception("Error parsing command library: " + msg)
//    }
//  }
//  private val compiledLib:List[CommandInstruction] = {
//      parsedLib map { let => CommandInstruction(let.name, compiler.compile(let)) }
//  }
//  def commands = compiledLib

  def apply(code: String): String = substitudeCommands(code)


  def substitudeCommands(code: String): String = {
    val parsedCmds: List[List[String]] = lib.split("\n").toList.map(_.split("=").toList)
    val cmds: List[Pair[String, String]] = parsedCmds collect {
      case name :: cmd :: Nil => Pair(name.trim, cmd.trim)
    }

    var tCode = code

    var modified = true

    while(modified) {
      modified = false
      for((name, cmd) <- cmds) {
        if(tCode.contains(name)) {
          tCode = tCode.replaceAll(name, s"($cmd)")
          modified = true
        }
      }
    }
    tCode
  }

  def lib =
    """
    """.stripMargin
//  def lib =
//    """
//      |zero =  λs.λz.z
//      |one =   λs.λz.s z
//      |two =   λs.λz.s (s z)
//      |three = λs.λz.s (s (s z))
//      |four =  λs.λz.s (s (s (s z)))
//      |five =  λs.λz.s (s (s (s (s z))))
//      |plus = λm.λn.λs.λz.m f (n s z)
//      |minus = λm.λn.(n pred) m
//      |succ = λn.λs.λz.f (n s z)
//      |mult = λm.λn.λs.m (n s)
//      |exp = λm.λn.n m
//      |pred = λn.λs.λz.n (λg.λh. h (g f))(λu.x)(λu.u)
//      |true =  λt.λf.t
//      |false = λt.λf.f
//      |if = λp.λa.λb. p a b
//      |and = λp.λq.p q p
//      |or = λp.λq.p p q
//      |not = λp.p false true
//    """.stripMargin
}
