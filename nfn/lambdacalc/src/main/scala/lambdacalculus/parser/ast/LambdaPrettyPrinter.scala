package lambdacalculus.parser.ast


/**
 * Created by basil on 08/05/14.
 */
object LambdaPrettyPrinter extends LambdaPrinter {

  def print(expr: Expr): String =  expr match {
    case Clos(arg, body) => s"λ$arg." + p"$body"
    case Application(fun, arg) => p"$fun $arg"
    case UnaryExpr(op, v) => s"$op " + p"$v"
    case BinaryExpr(op, v1, v2) => op.toString + p" $v1" + p" $v2"
    case Variable(name, _) => name
    case Constant(i) => i.toString
    case Let(name, expr, maybeCodeExpr) =>
      s"\nlet $name = " + p"$expr" + " endlet\n" + maybeCodeExpr.fold("")(e => p"$e")
    case IfElse(test, thenn, otherwise) => p"if $test \nthen $thenn \nelse $otherwise\n"
    case NopExpr() => "-"
    case Call(name, args) => {
      val s1 = s"(call ${args.size + 1} $name"
      val s2 = if(args.size > 0) {
          " " +
          args.map(apply).mkString(" ")
        } else ""
      val s3 = ")"

      s1 + s2 + s3
    }
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
