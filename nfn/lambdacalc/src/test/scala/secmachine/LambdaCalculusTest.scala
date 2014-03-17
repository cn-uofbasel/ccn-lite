package secmachine

import org.scalatest._
import secmachine.machine.{ConstValue, Value}
import ExecutionOrder._

import org.scalatest.TryValues._
import scala.util.{Failure, Success, Try}
import secmachine.machine.CallByValue.ClosureValue
import secmachine.parser.ast.Expr

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
    s"The expression: $expr" should s"evaluate with call-by-name to $result" in {
      LambdaCalculus(CallByName).substituteParseCompileExecute(expr) should be (tryConst(result))
    }
    it should s"evaluate with call-by-value to $result" in {
      LambdaCalculus(CallByValue).substituteParseCompileExecute(expr) should be (tryConst(result))
    }
    ignore should "compile and decompile to the same AST with call-by-name" in {
      val (parsed, decompiled) = decompile(CallByName)
      parsed shouldBe decompiled
    }
    ignore should "compile and decompile to the same AST with call-by-value" in {
      val (parsed, decompiled) = decompile(CallByValue)
      parsed shouldBe decompiled
    }
  }

  // POS
  testExpression("2", 2)
  testExpression("1 ADD 2", 3)
  testExpression("1 ADD -2", -1)
  testExpression("1 SUB 2", -1)
  testExpression("2 MULT 3", 6)
  testExpression("4 DIV 2", 2)
  testExpression("4 DIV 3", 1)
  testExpression("(1 ADD 2)", 3)
  testExpression("(1 ADD 2) SUB 3", 0)
  testExpression("1 ADD (2 SUB 3)", 0)
  testExpression("1 ADD 2 SUB 3", 0)
  testExpression("((λy.(λx.y ADD x)) 2) 3", 5)
  testExpression("(λy.(λx.x ADD x)) 2 3", 6)
  testExpression("(λx.(((λx.x ADD 1) 3) ADD x)) 7", 11)
  testExpression("((λx.x ADD 1) 2)", 3)
  testExpression("(λx. x ADD x) 3", 6)
  testExpression("one ADD one", 2)
  testExpression("succ 2", 3)
  testExpression("succ 3", 4)
  testExpression("if 2 GT 1 then 42 else 0", 42)
  testExpression("if 0 GT 1 then 42 else 0", 0)
  testExpression("(λx.if x GT 1 then 42 else 0) 2", 42)
  testExpression("(λx.if x GT 1 then 42 else 0) 0", 0)
  testExpression("(λx.if x LT 1 then 42 else 0) 0", 42)
  testExpression("let mySucc = (λx. x ADD 1) endlet mySucc 1", 2)
  testExpression("let f1 = (λa.a ADD 12) endlet let f2 = (λb.b SUB 12) endlet f1 42", 54)
  testExpression("let f1 = (λa.a ADD 12) endlet let f2 = (λb.b SUB 12) endlet f2 42", 30)
  testExpression("(λx. let f1 = (λa.a ADD 12) endlet let f2 = (λb.b SUB 12) endlet f2 x) 42 ", 30)
  testExpression("""
                   | (λx.
                   |   let f1 = (λa.a ADD 12) endlet
                   |   let f2 = (λb.b SUB 12) endlet
                   |   f2 x
                   | ) 42
                 """.stripMargin, 30)
  testExpression(
    """
      |let countToTen =
      |  λx.
      |    if (x LT 10)
      |    then
      |      countToTen (x ADD 1)
      |    else
      |      x
      |endlet
      |countToTen 10
    """.stripMargin, 10)
  testExpression(
    """
      |let countToTen =
      |  λx.
      |    if (x LT 10)
      |    then
      |      countToTen (x ADD 1)
      |    else
      |      x
      |endlet
      |countToTen 9
    """.stripMargin, 10)
  testExpression(
    """
      |let countToTen =
      |  λx.
      |    if (x LT 10)
      |    then
      |      countToTen (x ADD 1)
      |    else
      |      x
      |endlet
      |countToTen 8
    """.stripMargin, 10)

  // NEG

  "The expression: 1 ADD " should s" throw ParseException with call-by-value " in {
    evaluating {
      intercept[ParseException] {
        LambdaCalculus(CallByValue).substituteParseCompileExecute("1 ADD")
      }
    }

  }
  it should s" throw ParseException with call-by-name" in {
    evaluating {
      intercept[ParseException] {
        LambdaCalculus(CallByValue).substituteParseCompileExecute("1 ADD")
      }
    }
  }
}
