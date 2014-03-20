package lambdacalculus.machine

import lambdacalculus.machine.CallByValue._
import lambdacalculus.machine.CallByName._
import lambdacalculus.compiler._
import lambdacalculus.parser.ast.LambdaPrettyPrinter

trait Value {
  def maybeName: Option[String]
  override def toString: String = ValuePrettyPrinter.apply(this, None)
}

case class ConstValue(n: Int, maybeName: Option[String] = None) extends Value
case class CodeValue(c: List[Instruction], maybeName:Option[String] = None) extends Value

object ValuePrettyPrinter {

  def apply(value: Value, maybeCompiler: Option[Compiler] = None): String = {
    def instructionsToString(instr: List[Instruction]): String = maybeCompiler match {
        case Some(comp) => LambdaPrettyPrinter(comp.decompile(instr))
        case None => instr.mkString(",")
    }

    value match {
      case cbnValue: CBNValue => cbnValue match {
        case ClosureThunk(c, e, _) =>  {
          s"ClosThunk{ c: ${instructionsToString(c)} | e: $e }"
        }
      }
      case cbvValue: CBVValue => cbvValue match {
        case ClosureValue(n, c, e, _) => s"ClosVal{ c: ('${instructionsToString(c)}' == $c | e: (${e.mkString(",")}) }"
        case EnvValue(e, _) =>  s"EnvVal(${e.mkString(",")})"
        case VariableValue(n, _) => s"$n"
      }
      case v: Value => v match {
        case ConstValue(n, maybeVarName) => s"$n(${maybeVarName.getOrElse("-")})"
        case CodeValue(c, maybeVarName) => s"CodeVal(${c.mkString(",")}, ${maybeVarName.getOrElse("-")}))"
      }
      case _ => throw new NotImplementedError(s"ValuePrettyPrinter has no conversion for value $value")
    }
  }

}
