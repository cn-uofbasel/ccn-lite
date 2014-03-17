package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 14:17
 * To change this template use File | Settings | File Templates.
 */

class Substitution(argV: Var, replacement: Expr) {
  val binder = new Binder(Map())

  def apply(term: Expr): Expr = term match {
    case Var(argV.name, argV.scope) => binder.bind(replacement, argV.scope.parent.get)
    case Var(_, _)                  => term
    case Apply(fun, arg)            => Apply(apply(fun), apply(arg))
    case Lambda(arg, body)          => Lambda(arg, apply(body))
  }
}
