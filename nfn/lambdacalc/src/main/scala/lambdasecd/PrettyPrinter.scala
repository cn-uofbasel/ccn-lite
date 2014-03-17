package lambdasecd

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 27/11/13
 * Time: 14:33
 * To change this template use File | Settings | File Templates.
 */

class PrettyPrinter {
  def apply(expr: Expr): String = expr match {
    case Lambda(arg, body)       => s"λ$arg.($body)"
    case Constant(i)             => s"$i"
    case PredefinedFunction(fun) => s"$fun"
    case UnaryMultFunction(i)    => s"mult$i"
    case Apply(fun, arg)         => s"($fun $arg)"
    case Variable(name)          => s"$name"
    case Closure(variable, body, env) => s"[v:$variable, b:$body, e:" + Helper.envAsString(env) + "]"
    case ApplySymbol()           =>  "@"
  }
}


