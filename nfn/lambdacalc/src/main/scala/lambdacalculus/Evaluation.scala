package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 13:58
 * To change this template use File | Settings | File Templates.
 */

class Evaluation(debug: Boolean = false) {
  lazy val pretty = new PrettyPrinter()

  def apply(term: Expr): Expr =
    try {
      val term2 = evalStep(term)
      if(debug)
        println(s"step: ${pretty(term)} → ${pretty(term2)}")
      apply(term2)
    } catch {
      case _: MatchError => term
    }

  def evalStep(term: Expr): Expr = term match {
    case Apply(Lambda(argDef, body), arg) if isValue(arg) =>
      new Substitution(argDef, arg).apply(body)
    case Apply(fun, arg) if isValue(fun) =>
      Apply(fun, evalStep(arg))
    case Apply(fun, arg) =>
      Apply(evalStep(fun), arg)
  }

  def isValue(term: Expr): Boolean = term match {
    case _: Lambda => true
    case _: Var    => true
    case CNumber(_) => true
    case _         => false
  }
}


