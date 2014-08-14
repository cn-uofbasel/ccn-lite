package nfn

import lambdacalculus.parser.ast._
import ccn.packet.{CCNName, NFNInterest, Interest}

object LambdaNFNImplicits {
  import LambdaDSL._

  implicit def ccnNameToString(name: CCNName): Expr = name.toString

  implicit def ccnNamesToStirngs(names: List[CCNName]): List[Expr] = names map {_.toString}

  implicit def exprToString(expr: Expr)(implicit lambdaPrinter: LambdaPrinter): String = {
    lambdaPrinter(expr)
  }

  implicit def exprToInterest(expr: Expr)(implicit lambdaPrinter: LambdaPrinter): Interest = {
    expr match {
      case Variable(name, _) => Interest(name)
      case _ => NFNInterest(expr)
    }
  }
  implicit val lambdaPrinter: LambdaPrinter = LambdaNFNPrinter
}

/**
 * Created by basil on 08/05/14.
 */
object LambdaNFNPrinter extends LambdaPrinter {
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

  def print(expr: Expr): String =  {
//      LambdaNFNPreprocessor.apply(expr) match {
      expr match {
        case Clos(arg, body) => s"@$arg " + p"$body"
        case Application(fun, arg) => p"$fun $arg"
        case UnaryExpr(op, v) =>  op.toString.toLowerCase + " " + p"$v"
        case BinaryExpr(op, v1, v2) => op.toString.toLowerCase + " " + p"$v1" + " " + p"$v2"
        case Variable(name, _) => name
        case Constant(i) => i.toString
        case Let(name, expr, maybeCodeExpr) =>
          s"let $name = " + p"$expr" + " endlet" + maybeCodeExpr.fold("")(e => p"$e")
        case IfElse(test, thenn, otherwise) => "ifelse " + p"$test " + p"$thenn " + p"$otherwise"
        case Call(name, args) => s"call ${args.size + 1} $name" + (if(args.size > 0) " " else "") + args.map({arg: Expr => p"$arg"}).mkString(" ")
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
