package secmachine.parser.ast

import scala.util.parsing.input.Positional


object BinaryOp extends Enumeration {
  type BinaryOp = Value
  val Add = Value("ADD")
  val Sub = Value("SUB")
  val Mult = Value("MULT")
  val Div = Value("DIV")
  val Eq = Value("EQ")
  val Neq = Value("NEQ")
  val Lt = Value("LT")
  val Gt = Value("GT")
  val Lte = Value("LTE")
  val Gte = Value("GTE")
}

object UnaryOp extends Enumeration {
  type UnaryOp = Value
  val Cdr = Value("CDR")
}


sealed abstract class Expr extends Positional

case class Clos(boundVariable: String, body: Expr) extends Expr {
  override def equals(o: Any) = o match {
    case that: Clos => that.body == body
    case _ => false
  }
  override def hashCode = body.hashCode
}
case class Application(rator: Expr, rand: Expr) extends Expr

case class Variable(name: String, accessValue: Int = -1) extends Expr
case class Constant(i: Int) extends Expr
case class UnaryExpr(op: UnaryOp.UnaryOp, v: Expr) extends Expr
case class BinaryExpr(op: BinaryOp.BinaryOp, v1: Expr, v2:Expr) extends Expr
case class Let(name: String, fun: Expr, code: Option[Expr]) extends Expr
case class IfElse(test: Expr, thenn: Expr, otherwise: Expr) extends Expr
case class NopExpr() extends Expr


object LambdaPrettyPrinter {

  def apply(expr: Expr): String =  expr match {
    case Clos(arg, body) => s"λ$arg." + p"$body"
    case Application(fun, arg) => p"$fun $arg"
    case UnaryExpr(op, v) => s"$op " + p"$v"
    case BinaryExpr(op, v1, v2) => p"$v1 " + op.toString + p" $v2"
    case Variable(name, _) => name
//    case Constant(i) => i.toString
    case Let(name, expr, maybeCodeExpr) =>
      s"\nlet $name = " + p"$expr" + " endlet\n" + maybeCodeExpr.fold("")(e => p"$e")
    case IfElse(test, thenn, otherwise) => p"if $test \nthen $thenn \nelse $otherwise\n"
    case NopExpr() => "-"
    case _ => throw new NotImplementedError(s"LambdaPrettyPrinter cannot pretty print: $expr")
  }

  implicit class PrettyPrinting(val sc: StringContext) {
    def p(args: Expr*) = sc.s(args map parensIfNeeded:_*)
  }

  def parensIfNeeded(expr: Expr) = expr match {
    case Variable(name, _) => name
    case Constant(i) => i
//    case Literal(lit) => lit
    case _            => "(" + apply(expr) + ")"
  }
}


