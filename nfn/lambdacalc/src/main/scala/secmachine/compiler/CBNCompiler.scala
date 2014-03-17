package secmachine.compiler

import secmachine.parser.ast._
import secmachine.machine.CallByName._
import secmachine.machine._

case class CBNCompiler(override val debug: Boolean = false) extends Compiler {
  override def compileSpecific(ast: Expr): List[Instruction] = ast match {
    case Clos(boundVar, body) => GRAB() :: compile(body)
    case Application(rator: Expr, rand: Expr) => PUSH(compile(rand)) :: compile(rator)
    case Variable(name: String, accessValue) => (if(accessValue >= 0) ACCESSBYNAME(accessValue, name) else ACCESSBYNAME(0, name)) :: Nil
    case _ => throw new CompileException(s"CBNCompiler has no compilation for expression: $ast")
  }

//  override def decompileSpecific(instructions: List[Instruction], exprList: List[Expr]): (Expr, List[Instruction]) = {
//    instructions match {
//      case PUSH(rand) :: tail => {
//        println("PUSH")
//        val (decompiledRand, randTail) = decompileReverse(rand.reverse, List())
////        val (decompiledRator, ratorTail) = decompileReverse(tail, exprList)
//        decompileReverse(tail, Application(exprList.head, decompiledRand) :: exprList.tail)
//      }
//      case ACCESSBYNAME(_, name) :: tail =>  {
//        Variable(name, -1) -> tail
//      }
//      case GRAB() :: tail =>  {
////        val (decompiledBody, bodyTail) = decompileReverse(tail, exprList)
//        decompileReverse(tail, Clos("???", exprList.head) :: exprList.tail)
//      }
//      case _ => (exprList.head, instructions)
//    }
//  }

    override def decompileSpecific(instructions: List[Instruction]): (Expr, List[Instruction]) = ???
//  override def decompile(instructions: List[Instruction]): Expr = {
//    CBNDecompiler().decompile(instructions)
//  }
}


//case class CBNDecompiler() extends Decompiler() {
//  lazy val specific: P[Expr] = application | variable | closure
//
////  implicit def accept(p: PUSH): Parser[Expr] = expr(p)
//
//
//  lazy val application: P[Application] = push ~ expr ^^ { case (rator ~ rand) => Application(rator, rand)}
//  lazy val push: P[Expr] = acceptIf(_.isInstanceOf[PUSH])(_.toString) ^^ { case p: PUSH => decompile(p.c) }
//  lazy val variable: P[Variable] = acceptIf(_.isInstanceOf[ACCESSBYNAME])(_.toString) ^^ { case v: ACCESSBYNAME => Variable(v.storeName, -1)}
//  lazy val closure: P[Clos] = acceptIf(_.isInstanceOf[GRAB])(_.toString) ~> expr ^^ { case e => Clos("???", e)}
//}