package lambdacalculus.machine

import lambdacalculus.machine.CallByValue._
import lambdacalculus.machine.CallByName._
import lambdacalculus.compiler._
import lambdacalculus.parser.ast.LambdaPrettyPrinter

trait Value {
  def maybeContextName: Option[String]
  override def toString: String = ValuePrettyPrinter.apply(this, None)
}

case class ConstValue(n: Int, maybeContextName: Option[String] = None) extends Value
case class CodeValue(c: List[Instruction], maybeContextName:Option[String] = None) extends Value
case class ListValue(values:Seq[Value], maybeContextName: Option[String] = None) extends Value
case class NopValue() extends Value{
  override def maybeContextName: Option[String] = None
}

object ValuePrettyPrinter {
  def apply(value: Value, maybeCompiler: Option[Compiler] = None): String = {
    def instructionsToString(instr: List[Instruction]): String = maybeCompiler match {
        case Some(comp) => LambdaPrettyPrinter(comp.decompile(instr))
        case None => instr.mkString(",")
    }

    def throwNotImplementedError = throw new NotImplementedError(s"ValuePrettyPrinter has no conversion for value of type ${value.getClass.getCanonicalName}")

    value match {
      case cbnValue: CBNValue => cbnValue match {
        case ClosureThunk(c, e, _) =>  {
          s"ClosThunk{ c: ${instructionsToString(c)} | e: $e }"
        }
        case _ => throwNotImplementedError
      }
      case cbvValue: CBVValue => cbvValue match {
        case ClosureValue(n, c, e, _) => s"ClosVal{ c: ('${instructionsToString(c)}' == $c | e: (${e.mkString(",")}) }"
        case EnvValue(e, _) =>  s"EnvVal(${e.mkString(",")})"
        case VariableValue(n, _) => s"$n"
        case _ => throwNotImplementedError
      }
      case v: Value => v match {
        case ConstValue(n, maybeVarName) => s"$n(${maybeVarName.getOrElse("-")})"
        case CodeValue(c, maybeVarName) => s"CodeVal(${c.mkString(",")}, ${maybeVarName.getOrElse("-")}))"
        case ListValue(values, _) => values.mkString("[" , ", ", "]")
        case _ => throwNotImplementedError
      }
      case _ => throwNotImplementedError
    }
  }

}
