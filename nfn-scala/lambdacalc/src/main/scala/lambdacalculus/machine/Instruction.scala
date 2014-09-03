package lambdacalculus.machine

import scala.util.parsing.combinator.token.Tokens
import lambdacalculus.parser.ast.BinaryOp
import lambdacalculus.parser.ast.UnaryOp


trait Instruction extends Tokens {
  def stringRepr: String
}

case class LET(defName: String, clos: List[Instruction]) extends Instruction {
  def stringRepr: String = s"LET(${clos.mkString(",")}"
}

case class IF(test: List[Instruction]) extends Instruction {
  def stringRepr: String = s"IF(${test.mkString(", ")}"
}
case class THENELSE(thenn: List[Instruction], otherwise: List[Instruction]) extends Instruction {
  def stringRepr: String = s"THEN (${thenn.mkString(", ")} ELSE (${otherwise.mkString(",")})"
}

case class CONST(n: Int) extends Instruction {
  def stringRepr: String = s"CONST($n)"
}

case class Unary(op: UnaryOp.UnaryOp) extends Instruction {
  def stringRepr = s"$op"
  def apply(v: Value):Value = ??? //op(v)
}

case class BINARYOP(op: BinaryOp.BinaryOp) extends Instruction {
  implicit def boolToInt(b: Boolean): Int = if(b) 1 else 0

  def stringRepr = s"$op"

  def apply(v1: Value, v2: Value):Value = {
    import BinaryOp._
    (v1, v2) match {
      case (ConstValue(c1, _), ConstValue(c2, _)) => ConstValue(
        op match {
          case Add => c1 + c2
          case Sub => c1 - c2
          case Mult => c1 * c2
          case Div => c1 / c2
          case Eq => c1 == c2
          case Neq => c1 != c2
          case Gt => c1 > c2
          case Lt => c1 < c2
          case Gte => c1 >= c2
          case Lte => c1 <= c2
        }
      )
      case _ => throw new Exception(s"Operation ${op.toString} can only be applied to ConstValues and not v1: $v1 v2: $v2")
    }
  }
}

case class CALL(name: String, nArgs: Int) extends Instruction {
  override def stringRepr: String = s"CALL($name: $nArgs)"
}

