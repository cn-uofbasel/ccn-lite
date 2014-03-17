package secdmachine

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 15:21
 * To change this template use File | Settings | File Templates.
 */

sealed abstract class Instruction(var name: String) {
  override def toString() = name
}
// NIL
case class NIL() extends Instruction("NIL")
// Load Constant
case class LDC(integer: Int) extends Instruction(s"LDC($integer)")
// Load from environment
case class LD(x: Int, y: Int) extends Instruction(s"LD($x, $y)")

case class Unary(op: UnaryOp) extends Instruction(op.toString)
case class Binary(op: BinaryOp) extends Instruction(op.toString)

// if-else: selects either first or second instruction block from code checking the bool value on top of the stack
// the current code is put onto the dump and the code is replaced with the instruction block
case class SEL() extends Instruction("SEL")
// replaces the current code, which was selected by SEL(), with the original code before the if-else
case class JOIN() extends Instruction("JOIN")

case class InstructionBlock(l: List[Instruction]) extends Instruction(l.mkString("(", ", ", ")"))
case class CellValueBlock(l: List[CellValue]) extends Instruction( l.mkString("(", ", ", ")"))
case class EnvBlock(l: List[List[CellValue]]) extends Instruction(Helper.envToString(l))

// Load function
case class LDF() extends Instruction("LDF")

// Apply function
case class AP() extends Instruction("AP")

// Return code
case class RTN() extends Instruction("RTN")

// Dummy environment

case class DUM() extends Instruction("DUM")
// Apply recursive function
case class RAP() extends Instruction("RAP")

case class WRITEC() extends Instruction("WRTC")


sealed abstract class UnaryOp(name: String) {
  override def toString() = name
}
case class Atom() extends UnaryOp("ATOM")
case class Null() extends UnaryOp("NULL")
// head
case class Car() extends UnaryOp("CAR")
// tail
case class Cdr() extends UnaryOp("CDR")


sealed abstract class BinaryOp(name: String)
// Prepend an item to a list and return list
case class Cons() extends BinaryOp("CONS")
case class Add() extends BinaryOp("ADD")
case class Sub() extends BinaryOp("SUB")
case class Mult() extends BinaryOp("MULT")
case class Div() extends BinaryOp("DIV")
case class Eq() extends BinaryOp("EQ")
case class Neq() extends BinaryOp("NEQ")
case class Gt() extends BinaryOp("GT")
case class Lt() extends BinaryOp("LT")
case class Gte() extends BinaryOp("GTE")
case class Lte() extends BinaryOp("LTE")


object Helper {
  def envToString(env: List[List[CellValue]]): String = {
    val sb = new StringBuilder
    sb ++= "("
    env.foldLeft(sb){ (sb, s) => sb append s.mkString("(", ", ", ")") }
    sb ++= ")"
    sb.toString()
  }
}

sealed abstract class CellValue

case class CInteger(i: Int) extends CellValue {
  override def toString() = i.toString
}
//case class CBoolean(b: Boolean) extends CellValue
case class CNil() extends CellValue {
  override def toString() = "nil"
}
case class CList(list:List[CellValue]) extends CellValue {
  override def toString() = list.mkString("(", ", " ,")")
}
case class Closure(fun: List[Instruction], env: List[List[CellValue]]) extends CellValue {
  override def toString() = "[ f: (" + fun.mkString(", ") + "); e: " + Helper.envToString(env) + " ]"
}