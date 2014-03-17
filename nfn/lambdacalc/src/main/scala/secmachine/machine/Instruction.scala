package secmachine.machine

import scala.util.parsing.combinator.token.Tokens
import secmachine.parser.ast.BinaryOp
import secmachine.parser.ast.UnaryOp


trait Instruction extends Tokens {
//  override def toString = stringRepr
  def stringRepr: String
}

case class LET(clos: List[Instruction]) extends Instruction {
  def stringRepr: String = s"LET(${clos.mkString(",")}"
}

//case class ENDLET() extends Instruction {
//  def stringRepr: String = "ENDLET"
//}

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
      case (ConstValue(c1), ConstValue(c2)) => ConstValue(
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


//sealed abstract class UnaryOp(name: String) {
//  override def toString = name
//
//  def apply(v: Value):Value = ???
//}

//case class ATOM() extends UnaryOp("ATOM")

//case class NULL() extends UnaryOp("NULL")

//// head
//case class CAR() extends UnaryOp("CAR")
//
//// tail
//case class CDR() extends UnaryOp("CDR")

