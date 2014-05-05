package lambdacalculus.parser.ast

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
case class Let(name: String, letExpr: Expr, code: Option[Expr]) extends Expr
case class IfElse(test: Expr, thenn: Expr, otherwise: Expr) extends Expr
case class NopExpr() extends Expr
case class Call(name: String, args: List[Expr]) extends Expr


object LambdaLocalPrettyPrinter {

  def apply(expr: Expr): String =  expr match {
    case Clos(arg, body) => s"λ$arg." + p"$body"
    case Application(fun, arg) => p"$fun $arg"
    case UnaryExpr(op, v) => s"$op " + p"$v"
    case BinaryExpr(op, v1, v2) => p"$v1 " + op.toString + p" $v2"
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

//object LambdaNFNPreprocessor {
//  private def replaceLets(letName: String, letExpr: Expr, expr: Expr): Expr = {
//    def rpl(expr: Expr): Expr = {
//      expr match {
//        case clos@Clos(arg, body) =>
//          // Only replace the name if the closure does not introduce the same name
////          if (arg != letName)
//            Clos(arg, rpl(expr))
////          else clos
//        case Application(fun, arg) => Application(rpl(fun), rpl(arg))
//        case UnaryExpr(op, v) => UnaryExpr(op, rpl(v))
//        case BinaryExpr(op, v1, v2) =>  BinaryExpr(op, rpl(v1), rpl(v2))
//        case v @ Variable(name, _) => if(name == letName) letExpr else v
//        case c @ Constant(i) => c
//        case Let(name, expr, maybeCodeExpr) => Let(name, rpl(expr), maybeCodeExpr map { rpl })
//        case IfElse(test, thenn, otherwise) => IfElse(rpl(test), rpl(thenn), rpl(otherwise))
//        case Call(name, args) =>
//          if(name == letName)
//            Call(name, args map { rpl })
//          else
//            Call(name, args map { rpl })
//
//        case n @ NopExpr() => n
//        case _ => throw new NotImplementedError(s"LambdaPrettyPrinter cannot pretty print: $expr")
//      }
//    }
//    rpl(expr)
//  }
//
//
//  // Recursively goes througha all expressions and for each Let expression tries to replace all occurrences in the subtree
//  def apply(expr: Expr): Expr = expr match {
//    case let @ Let(name, letExpr, maybeCode) => {
//      maybeCode match {
//        case Some(code) => replaceLets(name, letExpr, code)
//        case None => let
//      }
//    }
//    case Clos(arg, body) => Clos(arg, apply(body))
//    case Application(fun, arg) => Application(apply(fun), apply(arg))
//    case UnaryExpr(op, v) => UnaryExpr(op, apply(v))
//    case BinaryExpr(op, v1, v2) =>  BinaryExpr(op, apply(v1), apply(v2))
//    case v:Variable => v
//    case c:Constant => c
//    case IfElse(test, thenn, otherwise) => IfElse(apply(test), apply(thenn), apply(otherwise))
//    case Call(name, args) => Call(name, args map { apply })
//    case n @ NopExpr() => n
//    case _ => expr
//  }
//}


object LambdaNFNPrinter {
//  def nfnNameComponents(expr: Expr): Array[String] = {
//
//    val (restExpr:Expr, firstCmp) = expr match {
//      case Clos(arg, body) => throw new Exception("LambdaNFNPrinter.nfnNameComponents cannot split a top-level closure to a nfn compatible name")
//      case Application(fun, arg) =>
//      case UnaryExpr(op, v) =>  op.toString.toLowerCase + " " + p"$v"
//      case BinaryExpr(op, v1, v2) => op.toString.toLowerCase + " " + p"$v1" + " " + p"$v2"
//      case Variable(name, _) => (NopExpr, name)
//      case Constant(i) => (NopExpr, i.toString)
//      //    case Let(name, expr, maybeCodeExpr) =>
//      //      s"\nlet $name = " + p"$expr" + " endlet\n" + maybeCodeExpr.fold("")(e => p"$e")
//      //    case IfElse(test, thenn, otherwise) => p"if $test \nthen $thenn \nelse $otherwise\n"
//      case NopExpr() => ("")
//      case _ => throw new NotImplementedError(s"LambdaPrettyPrinter cannot pretty print: $expr")
//    }
//  }

  def apply(expr: Expr): String =  {
//      LambdaNFNPreprocessor.apply(expr) match {
      expr match {
        case Clos(arg, body) => s"@$arg " + p"$body"
        case Application(fun, arg) => p"$fun $arg"
        case UnaryExpr(op, v) =>  op.toString.toLowerCase + " " + p"$v"
        case BinaryExpr(op, v1, v2) => op.toString.toLowerCase + " " + p"$v1" + " " + p"$v2"
        case Variable(name, _) => name
        case Constant(i) => i.toString
        case Let(name, expr, maybeCodeExpr) =>
          s"\nlet $name = " + p"$expr" + " endlet\n" + maybeCodeExpr.fold("")(e => p"$e")
            case IfElse(test, thenn, otherwise) => p"if $test \nthen $thenn \nelse $otherwise\n"
        case Call(name, args) => s"call ${args.size + 1} $name " + args.map({arg: Expr => p"$arg"}).mkString(" ")
        case NopExpr() => ""
        case _ => throw new NotImplementedError(s"NFNPrettyPrinter cannot pretty print: $expr")
      }
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
