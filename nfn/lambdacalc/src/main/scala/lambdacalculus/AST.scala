package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 13:58
 * To change this template use File | Settings | File Templates.
 */

import scala.util.parsing.input.Positional

sealed trait Expr extends Positional

case class Lambda(arg: Var, body: Expr) extends Expr

object Var{
  def apply(name: String): Var = Var(name, Scope.TOP)
}
case class Var(name: String, scope: Scope) extends Expr

case class Apply(fun: Expr, arg: Expr) extends Expr
