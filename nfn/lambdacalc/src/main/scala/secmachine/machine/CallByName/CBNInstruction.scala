package secmachine.machine.CallByName

import secmachine.machine._

sealed trait CBNInstruction extends Instruction

//case class GRAB(code: List[Instruction]) extends CBNInstruction {
//  def stringRepr: String = s"GRAB(${code.mkString(",")})"
//}
case class GRAB() extends CBNInstruction {
  def stringRepr: String = s"GRAB"
}
case class PUSH(c: List[Instruction]) extends CBNInstruction {
  def stringRepr: String = c.mkString("PUSH(",",", ")")
}
case class ACCESSBYNAME(i: Int, storeName: String) extends CBNInstruction {
  def stringRepr: String = s"ACCESS_$storeName($i)"
}


