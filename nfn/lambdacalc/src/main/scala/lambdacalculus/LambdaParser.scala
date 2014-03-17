package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 13:56
 * To change this template use File | Settings | File Templates.
 */

import scala.util.parsing.combinator.lexical.StdLexical
import scala.util.parsing.combinator.PackratParsers
import scala.util.parsing.combinator.syntactical.{StandardTokenParsers, StdTokenParsers}

class LambdaLexer extends StdLexical {
  override def letter = elem("letter", c => c.isLetter && c != 'λ' && c != '/')
}

class LambdaParser extends StdTokenParsers with PackratParsers {
  type Tokens = StdLexical
//  override type Elem = Expr
  val lexical =  new LambdaLexer
  lexical.delimiters ++= Seq("\\", "/", "λ", ".", "(", ")", "=", ";")

  type P[+T] = PackratParser[T]

  lazy val expr:        P[Expr]   = application | notApp
  lazy val notApp                 = variable | number | parens | lambda
  lazy val lambda:      P[Lambda] = positioned(("λ" | "\\" | "/")  ~> variable ~ "." ~ expr ^^
                                    { case v ~ "." ~ e => Lambda(v, e) })
  lazy val application: P[Apply]  = positioned(expr ~ notApp ^^
                                    { case left ~ right => Apply(left, right) })
  lazy val variable:    P[Var]    = positioned(ident ^^ Var.apply)
  lazy val parens:      P[Expr]   = "(" ~> expr <~ ")"
  lazy val number:      P[Lambda] = numericLit ^^ { case n => CNumber(n.toInt) }

  def parse(str: String) = {
    val tokens = new lexical.Scanner(str)
    phrase(expr)(tokens)
  }

  lazy val defs = repsep(defn, ";") <~ opt(";") ^^ Map().++
  lazy val defn = ident ~ "=" ~ expr  ^^ { case id ~ "=" ~ t => id -> t}

  def definitions(str: String): ParseResult[Map[String, Expr]] = {
    val tokens = new lexical.Scanner(str)
    phrase(defs)(tokens)
  }

}
