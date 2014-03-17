package secmachine.compiler

import secmachine.parser.ast._
import secmachine.machine._
import com.typesafe.scalalogging.slf4j.Logging

case class CompileException(msg: String) extends Exception(msg)
case class DecompileException(msg: String) extends Exception(msg)

trait Compiler extends Logging{

  def apply(ast: Expr): List[Instruction] = {
    logger.info(s"Compiling by name: $ast")

    val boundAST = Binder.apply(ast)
    if(debug) logger.debug(s"Bound AST: $boundAST")

    compile(boundAST)
  }

  implicit def instructionToList(instruction: Instruction): List[Instruction] = instruction :: Nil
  implicit def exprToInstruction(ast: Expr): List[Instruction] = compile(ast)
//  implicit def instructionsToExpr(instructions: List[Instruction]): Expr = decompile(instructions)

  def compile(ast: Expr): List[Instruction] = {
  ast match {
    case Constant(n: Int) => CONST(n)
    case UnaryExpr(op, v) => v ++ Unary(op)
    case BinaryExpr(op, v1, v2) => v2 ++ v1 ++ BINARYOP(op)
    case Let(_, fun, program) => program match {
      case Some(prog) => LET(fun) ++ prog
      case None => LET(fun)
    }
    case IfElse(test, thenn, otherwise) => {
      IF(test) :: THENELSE(thenn, otherwise) :: Nil
    }
    case _ => compileSpecific(ast)
  }
  }

//  def decompile(instructions: List[Instructions]): Expr = {
//    val (expr, restWithLet) = decompileWithoutLet(instructions)
//
//  }

  protected def compileSpecific(ast: Expr): List[Instruction]

  protected def decompileSpecific(instructions: List[Instruction]): (Expr, List[Instruction])

  def debug: Boolean

  def decompile(instructions:List[Instruction]): Expr = {
//    var rest:List[Instruction] = Nil
//    var expr:Expr = null
    val (expr, rest) = decompileReverse(instructions.reverse)
    if(!rest.isEmpty) logger.warn(s"Could not fully decompile: $instructions! Reamining part: $rest ")
//    expr = res._1
//    rest = res._2
//    while(!rest.isEmpty) {
//      val res = decompileReverse(rest, List(expr))
//      expr = res._1
//      rest = res._2
//    }
    expr
  }

  protected def decompileReverse(instructions: List[Instruction]): (Expr, List[Instruction]) = {

//    implicit def decompileUnaryLiteral(op: UnaryOp): Literal = ???

    instructions match {
      case BINARYOP(op) :: tail => {
        val (v1, v1Tail) = decompileReverse(tail)
        val (v2, v2Tail) = decompileReverse(v1Tail)

        BinaryExpr(op, v1, v2) -> v2Tail
      }
      case Unary(op) :: tail => {
//        val (v, unaryTail) = decompileReverse(tail, exprList)
//        UnaryExpr(op, v) -> unaryTail
        ???
      }
      case CONST(n) :: tail =>  Constant(n) -> tail
      case LET(fun) :: prog => {
        val (decompiledFun, funTail) = decompileReverse(fun.reverse)
        val (decompiledProg, progTail) = decompileReverse(prog)

        Let("???", decompiledFun, Some(decompiledProg)) -> progTail
      }
//      case ENDLET() :: funTail => decompileReverse(funTail, exprList)
      case _ => decompileSpecific(instructions)
    }
  }
}
//  def applyWithCommands(ast: Expr): List[Instruction] = {
//    info(s"Compiling by name: $ast")
//
//    val cmds = CommandLibrary(this).commands
//
//    val boundAST = Binder.applyWithCommands(ast, cmds)
//    if(debug) debug(s"Bound AST: $boundAST")
//
//    compile(boundAST)
//  }

//protected def decompileReverse(instructions: List[Instruction], exprList: List[Expr]): (Expr, List[Instruction]) = {
//if(exprList.size > 1) println(s">>>>>>>$exprList")
//implicit def decompileLiteral(op: BinaryOp): Literal = Literal( op match {
//case ADD() => "ADD"
//case SUB() => "SUB"
//case MULT() => "MULT"
//case DIV() => "DIV"
//case EQ() => "EQ"
//case NEQ() => "NEQ"
//case GT() => "GT"
//case GTE() => "GTE"
//case LT() => "LT"
//case LTE() => "LTE"
//})
//
//implicit def decompileUnaryLiteral(op: UnaryOp): Literal = ???
//
//instructions match {
//case Nil => exprList.head -> Nil
//case Binary(op) :: tail => {
//val (v1, v1Tail) = decompileReverse(tail, exprList)
//val (v2, v2Tail) = decompileReverse(v1Tail, exprList)
//
//decompileReverse(v2Tail, BinaryExpr(op, v1, v2) :: exprList)
//}
//case Unary(op) :: tail => {
////        val (v, unaryTail) = decompileReverse(tail, exprList)
////        UnaryExpr(op, v) -> unaryTail
//???
//}
//case CONST(n) :: Nil =>  Constant(n) -> Nil
//case LET(fun) :: prog => {
//val (decompiledFun, funTail) = decompileReverse(fun.reverse, exprList)
//val (decompiledProg, progTail) = decompileReverse(prog, exprList)
//
//decompileReverse(progTail, Let("???", decompiledFun, Some(decompiledProg)) :: exprList)
//}
//case ENDLET() :: funTail => decompileReverse(funTail, exprList)
//case _ => decompileSpecific(instructions, exprList)
//}

//case class InstructionSeqReader(instr: List[Instruction]) extends Reader[Instruction] {
//                                override val offset: Int) extends Reader[Instruction] {

//  def this(instr: List[Instruction]) = this(instr, 0)

//  override def atEnd: Boolean = offset >=  instr.length
//
//  override def pos: Position = NoPosition//new OffsetPosition(source, offset)
//
//  override def rest: Reader[Instruction] =
//    if(offset < instr.length)
//      InstructionSeqReader(instr, offset + 1)
//    else
//      this
//
//  override def first: Instruction = instr(offset)
//  override def atEnd: Boolean = instr.isEmpty
//
//  override def pos: Position = NoPosition//new OffsetPosition(source, offset)
//
//  override def rest: Reader[Instruction] = InstructionSeqReader(instr.tail)
//
//  override def first: Instruction = instr.head
//
//}
//
//trait Decompiler extends Parsers with PackratParsers {
//  type Elem = Instruction
//  type P[+T] = PackratParser[T]
//  implicit def decompileLiteral(op: BinaryOp): Literal = Literal( op match {
//    case ADD() => "ADD"
//    case SUB() => "SUB"
//    case MULT() => "MULT"
//    case DIV() => "DIV"
//    case EQ() => "EQ"
//    case NEQ() => "NEQ"
//    case GT() => "GT"
//    case GTE() => "GTE"
//    case LT() => "LT"
//    case LTE() => "LTE"
//  })
//
//  lazy val expr: P[Expr] = binary | constant //| specific
//  lazy val binary: P[BinaryExpr] = expr ~ expr ~ elem("binary", _.isInstanceOf[Binary]) ^^ { case (v1 ~ v2 ~ (bin: Binary)) => BinaryExpr(bin.op, v1, v2)}
//  lazy val constant: P[Constant] = elem("constant", _.isInstanceOf[CONST]) ^^ { case c: CONST => Constant(c.n)}
//  def specific: P[Expr]
//
//  def decompile(instr: List[Instruction]): Expr = {
//    phrase(expr)(InstructionSeqReader(instr)) match {
//      case Success(expr, next) => expr
//      case NoSuccess(err, next) => throw new DecompileException(err)
//    }
//  }
//
//  def apply(instr: List[Instruction]): Expr = decompile(instr)
//}
