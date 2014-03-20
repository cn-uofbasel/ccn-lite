package lambdacalculus.machine.CallByValue

import lambdacalculus.machine._


sealed trait CBVValue extends Value
case class ClosureValue(varName: String, c: List[Instruction], e: List[Value], maybeName: Option[String] = None) extends CBVValue
case class EnvValue(c: List[Value], maybeName: Option[String] = None) extends CBVValue

case class VariableValue(n: String, maybeName: Option[String] = None) extends CBVValue

