package secmachine.machine.CallByValue

import secmachine.machine._


sealed trait CBVValue extends Value
case class ClosureValue(c: List[Instruction], e: List[Value]) extends CBVValue
case class EnvValue(c: List[Value]) extends CBVValue

case class VariableValue(n: String) extends CBVValue

