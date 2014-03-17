package lambdacalculus.machine.CallByValue

import lambdacalculus.machine._

sealed trait CBVInstruction extends Instruction

case class ACCESSBYVALUE(i: Int, storeName: String) extends CBVInstruction {
  def stringRepr: String = s"ACCESS_$storeName($i)"
}


case class CLOSURE(boundVar: String, c: List[Instruction]) extends CBVInstruction {
  def stringRepr: String = c.mkString(s"CLOSURE($boundVar)[", ",", "]")
}
case class APPLY() extends CBVInstruction {
  def stringRepr: String = "APPLY"
}
case class RETURN() extends CBVInstruction {
  def stringRepr: String = "RETURN"
}
