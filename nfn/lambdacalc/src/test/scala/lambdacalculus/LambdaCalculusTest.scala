package lambdacalculus

import org.scalatest._
import lambdacalculus.machine.{ConstValue, Value}
import ExecutionOrder._

import scala.util.{Failure, Success, Try}
import lambdacalculus.parser.ast.Expr

/**
* Created with IntelliJ IDEA.
* User: basil
* Date: 02/12/13
* Time: 16:55
* To change this template use File | Settings | File Templates.
*/
class LambdaCalculusTest extends FlatSpec with Matchers with GivenWhenThen{

  def const(i: Int): List[Value] = List(ConstValue(i))
  def tryConst(i: Int): Try[List[Value]] = Try(const(i))

  def testExpression(expr: String, result: Int) = {
    def decompile(execOrder: ExecutionOrder): (Expr, Expr) = {
      val lc = LambdaCalculus(execOrder)
      val parsed = for {s <- lc.substituteLibrary(expr)
                        p <- lc.parse(s)}
      yield p
      val compiled = parsed.flatMap(lc.compile)

      When(s"Compiled: ${compiled}")
      val decompiled = compiled match {
        case Success(c) => lc.compiler.decompile(c)
        case Failure(e) => throw new Exception(s"Could not decompile $e")
      }
      (parsed.get, decompiled)
    }
    s"The expression: $expr" should s"evaluate with call-by-value to $result" in {
      val r = LambdaCalculus(CallByValue).substituteParseCompileExecute(expr)

      r.isSuccess should be (true)
      r.get.size should be (1)
      r.get.head should be (a [ConstValue])
      r.get.head.asInstanceOf[ConstValue].n should be (result)
    }
//    ignore should s"evaluate with call-by-name to $result" in {
//      val r = LambdaCalculus(CallByName).substituteParseCompileExecute(expr)
//
//      r.isSuccess should be (true)
//      r.get.size should be (1)
//      r.get.head should be (a [ConstValue])
//      r.get.head.asInstanceOf[ConstValue].n should be (result)
//    }
//    ignore should "compile and decompile to the same AST with call-by-name" in {
//      val (parsed, decompiled) = decompile(CallByName)
//      parsed shouldBe decompiled
//    }
//    ignore should "compile and decompile to the same AST with call-by-value" in {
//      val (parsed, decompiled) = decompile(CallByValue)
//      parsed shouldBe decompiled
//    }
  }

  val tupleEnv =
    """
      |let pair = λa.λb.λf.(f a b) endlet
      |let first  = λp.p (λa.λb.a) endlet
      |let second = λp.p (λa.λb.b) endlet
    """.stripMargin
  testExpression( s"$tupleEnv first (pair 1 2)", 1 )
  testExpression( s"$tupleEnv second (pair 1 2)", 2 )

  val boolEnv =
    """
      |let true = 1 endlet
      |let false = 0 endlet
    """.stripMargin

  val listEnv =
    s"""
      |$boolEnv
      |$tupleEnv
      |let empty = λf.λx.x endlet
      |let append = λa.λl.λf.λx.f a (l f x) endlet
      |let head = λl.l(λa.λb.a) dummy endlet
      |let isempty = λl.l (λa.λb.false) true endlet
      |let map = λl.λh.λf.λx.l ((λm.f m) h) x endlet
      |let tail = λl.first (l (λa.λb.pair (second b) (append a (second b)) ) (pair empty empty)) endlet
    """.stripMargin

  testExpression(s"$listEnv head (append 1 empty)", 1)

  testExpression(s"$listEnv head (append 2 (append 1 empty))", 2)

//  testExpression(s"$listEnv head (tail (append 2 (append 1 empty)))", 1)

  testExpression(s"$listEnv head (map  (append 2 (append 1 empty)) (λa.add a 1))", 3)

  // POS
  testExpression("2", 2)
  testExpression("add 1 2", 3)
  testExpression("add 1 -2", -1)
  testExpression("sub 1 2", -1)
  testExpression("mult 2 3", 6)
  testExpression("div 4 2", 2)
  testExpression("div 4 3", 1)
  testExpression("(add 1 2)", 3)
  testExpression("sub (add 1 2) 3", 0)
  testExpression("add 1 (sub 2 3)", 0)
  testExpression("sub add 1 2 3", 0)
  testExpression("((λy.(λx.add y x)) 2) 3", 5)
  testExpression("(λy.(λx.add x x)) 2 3", 6)
  testExpression("(λx.add ((λx.add x 1) 3) x ) 7", 11)
  testExpression("((λx.add x 1) 2)", 3)
  testExpression("(λx. add x x) 3", 6)
  testExpression("if gt 2 1 then 42 else 0", 42)
  testExpression("if gt 0 1 then 42 else 0", 0)
  testExpression("(λx.if gt x 1 then 42 else 0) 2", 42)
  testExpression("(λx.if gt x 1 then 42 else 0) 0", 0)
  testExpression("(λx.if lt x 1 then 42 else 0) 0", 42)
  testExpression("let mySucc = (λx. add x 1) endlet mySucc 1", 2)
  testExpression("let f1 = (λa.add a 12) endlet let f2 = (λb.sub b 12) endlet f1 42", 54)
  testExpression("let f1 = (λa.add a 12) endlet let f2 = (λb.sub b 12) endlet f2 42", 30)
  testExpression("(λx. let f1 = (λa.add a 12) endlet let f2 = (λb.sub b 12) endlet f2 x) 42 ", 30)
  testExpression("""
                   | (λx.
                   |   let f1 = (λa.add a 12) endlet
                   |   let f2 = (λb.sub b 12) endlet
                   |   f2 x
                   | ) 42
                 """.stripMargin, 30)
  testExpression(
    """
      |let countToTen =
      |  λx.
      |    if (lt x 10)
      |    then
      |      countToTen (add x 1)
      |    else
      |      x
      |endlet
      |countToTen 10
    """.stripMargin, 10)
  testExpression(
    """
      |let countToTen =
      |  λx.
      |    if (lt x 10)
      |    then
      |      countToTen (add x 1)
      |    else
      |      x
      |endlet
      |countToTen 9
    """.stripMargin, 10)
  testExpression(
    """
      |let countToTen =
      |  λx.
      |    if (lt x 10)
      |    then
      |      countToTen (add x 1)
      |    else
      |      x
      |endlet
      |countToTen 8
    """.stripMargin, 10)

  // NEG

  "The expression: 1 ADD " should s" throw ParseException with call-by-value " in {
    evaluating {
      intercept[ParseException] {
        LambdaCalculus(CallByValue).substituteParseCompileExecute("add 1")
      }
    }

  }
  it should s" throw ParseException with call-by-name" in {
    evaluating {
      intercept[ParseException] {
        LambdaCalculus(CallByValue).substituteParseCompileExecute("add 1 ")
      }
    }
  }
}
