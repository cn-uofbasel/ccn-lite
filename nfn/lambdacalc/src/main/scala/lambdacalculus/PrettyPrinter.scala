package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 14:00
 * To change this template use File | Settings | File Templates.
 */

class PrettyPrinter {
  def apply(expr: Expr): String = expr match {
    case Lambda(arg, body) => p"λ$arg.$body"
    case CNumber(i)        => i.toString()
    case CBoolean(b)       => b.toString()
    case Apply(fun, arg)   => p"$fun $arg"
    case Var(name, scope)  => s"$name"
  }

  implicit class PrettyPrinting(val sc: StringContext) {
    def p(args: Expr*) = sc.s((args map parensIfNeeded):_*)
  }

  def parensIfNeeded(expr: Expr) = expr match {
    case Var(name, _) => name
    case _            => "(" + apply(expr) + ")"
  }
}


