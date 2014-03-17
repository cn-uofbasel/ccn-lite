package secmachine.machine

import secmachine.machine.CallByValue._
import secmachine.machine.CallByName._
import secmachine.compiler._
import secmachine.parser.ast.LambdaPrettyPrinter

trait Value {
  override def toString: String = ValuePrettyPrinter.apply(this, None)
}

case class ConstValue(n: Int) extends Value
case class CodeValue(c: List[Instruction]) extends Value


object ValuePrettyPrinter {

  def apply(value: Value, maybeCompiler: Option[Compiler] = None): String = {
    def instructionsToString(instr: List[Instruction]): String = maybeCompiler match {
        case Some(comp) => LambdaPrettyPrinter(comp.decompile(instr))
        case None => instr.mkString(",")
    }

    value match {
      case cbnValue: CBNValue => cbnValue match {
        case ClosureThunk(c, e) =>  {
          s"ClosThunk{ c: ${instructionsToString(c)} | e: $e }"
        }
      }
      case cbvValue: CBVValue => cbvValue match {
        case ClosureValue(c, e) => s"ClosVal{ c: ('${instructionsToString(c)}' == $c | e: (${e.mkString(",")}) }"
        case EnvValue(e) =>  s"EnvVal(${e.mkString(",")})"
        case VariableValue(n) => s"$n"
      }
      case v: Value => v match {
        case ConstValue(n) => s"$n"
        case CodeValue(c) => s"CodeVal(${c.mkString(",")})"
      }
      case _ => throw new NotImplementedError(s"ValuePrettyPrinter has no conversion for value $value")
    }
  }

}
