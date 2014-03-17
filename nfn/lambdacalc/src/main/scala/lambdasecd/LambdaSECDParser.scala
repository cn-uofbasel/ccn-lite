package lambdasecd

import scala.util.parsing.combinator.lexical.StdLexical
import scala.util.parsing.combinator.PackratParsers
import scala.util.parsing.combinator.syntactical.StdTokenParsers

class LambdaLexer extends StdLexical {
  override def letter = elem("letter", c => c.isLetter && c != 'λ')
}

class LambdaSECDParser extends StdTokenParsers with PackratParsers {
  type Tokens = StdLexical
  val lexical =  new LambdaLexer
  lexical.delimiters ++= Seq("\\", "λ", ".", "(", ")")//, "=", ";")
  lexical.reserved += ("mult", "sqrt")

  type P[+T] = PackratParser[T]

  val predefinedFunctions = Set[Parser[String]]("mult", "sqrt").reduce(_ | _)

  lazy val expr:        P[Expr]   = application | notApp
  lazy val notApp                 = predFun | variable | number | parens | lambda
  lazy val lambda:      P[Lambda] = positioned(("λ" | "\\")  ~> ident ~ "." ~ expr ^^ { case name ~ "." ~ e => Lambda(name, e) })
  lazy val application: P[Apply]  = positioned(expr ~ notApp ^^ { case left ~ right => Apply(left, right) })
  lazy val parens:      P[Expr]   = "(" ~> expr <~ ")"
  lazy val predFun:     P[PredefinedFunction] = positioned( predefinedFunctions ^^ { case f => PredefinedFunction(f)} )
  lazy val variable:    P[Variable]    = positioned(ident ^^ Variable)
  lazy val number:      P[Constant] = numericLit ^^ { case n => Constant(n.toInt) }

  def parse(str: String) = {
    val tokens = new lexical.Scanner(str)
    phrase(expr)(tokens)
  }
//  lazy val defs = repsep(defn, ";") <~ opt(";") ^^ Map().++
//  lazy val defn = ident ~ "=" ~ expr  ^^ { case id ~ "=" ~ t => id -> t}
//
//  def definitions(str: String): ParseResult[Map[String, Expr]] = {
//    val tokens = new lexical.Scanner(str)
//    phrase(defs)(tokens)
//  }
}

