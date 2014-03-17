package lambdasecd

// TODO rename this file to AST

import scala.util.parsing.input.Positional
import scala.collection.immutable.ListMap

sealed abstract class Expr(repr: String) extends Positional {
  val pretty = new PrettyPrinter()

  override def toString() = pretty(this)
}

//case class Lambda(arg: Variable, body: Expr) extends Expr
case class Lambda(boundVariable: String, body: Expr) extends Expr(s"λ$boundVariable.($body)")
case class Apply(rator: Expr, rand: Expr) extends Expr(s"($rator $rand)")
case class Variable(name: String) extends Expr(s"$name")
case class Constant(i: Int) extends Expr(s"$i")
case class ApplySymbol() extends Expr("@")

case class Closure(boundVariable: String, body: Expr, env: List[ListMap[String, Expr]]) extends Expr(s"[v:$boundVariable, b:$body, e:$env]")

case class PredefinedFunction(name: String) extends Expr(s"$name") {
  def apply(Expr: Expr): Expr = name match {
    case "mult" => UnaryMultFunction(Expr.asInstanceOf[Constant].i)
    case "sqrt" => {
      val i = Expr.asInstanceOf[Constant].i
      Constant(i * i)
    }
  }
}
case class UnaryMultFunction(i: Int) extends Expr(s"$i") {
  def apply(c: Expr): Expr = Constant(c.asInstanceOf[Constant].i * i)
}


