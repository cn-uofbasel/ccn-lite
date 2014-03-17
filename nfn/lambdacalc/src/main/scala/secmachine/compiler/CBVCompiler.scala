package secmachine.compiler

import secmachine.machine.CallByValue._
import secmachine.parser.ast._
import secmachine.machine._

case class CBVCompiler(override val debug: Boolean = false) extends Compiler {
  override def compileSpecific(ast: Expr): List[Instruction] =  ast match {
    case Clos(boundVar, body) => CLOSURE(boundVar, body ++ RETURN())
    case Application(rator: Expr, rand: Expr) => rator ++ rand ++ APPLY()
    case Variable(name: String, accessValue) =>
      if(accessValue >= 0) {
        ACCESSBYVALUE(accessValue, name)
      } else {
        println(s"Unbound variable $name")
        ACCESSBYVALUE(0, name)
      }
    case _ => throw new CompileException(s"CBVCompiler has no compilation for expression: $ast")
  }

//  override def decompileSpecific(instructions: List[Instruction], exprList: List[Expr]): (Expr, List[Instruction]) = ???
  override def decompileSpecific(instructions: List[Instruction]): (Expr, List[Instruction]) = {
    instructions match {
      case APPLY() :: applyTail  => {
        val (rator, ratorTail) = decompileReverse(applyTail)
        val (rand, randTail) = decompileReverse(ratorTail)
       Application(rand, rator) -> randTail
      }
      case CLOSURE(boundVar, codeList) :: closTail => {
        val (code, codeTail) = decompileReverse(codeList.reverse)
        Clos(boundVar, code) -> closTail
      }
      case RETURN() :: returnTail => decompileReverse(returnTail)
      case ACCESSBYVALUE(_, name) :: tail => Variable(name) -> tail
      case _ => throw new DecompileException(s"CBVCompiler has no decompilation for insructions: $instructions")
    }
  }
}

