package lambdacalculus.parser.ast

/**
 * Created by basil on 08/05/14.
 */
trait LambdaPrinter {

  def apply(expr: Expr): String = print(expr)
  def print(expr: Expr): String
}
