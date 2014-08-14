package lambdacalculus.machine.CallByValue

import lambdacalculus.machine._


sealed trait CBVValue extends Value
case class ClosureValue(varName: String, c: List[Instruction], e: List[Value], maybeContextName: Option[String] = None) extends CBVValue
case class EnvValue(c: List[Value], maybeContextName: Option[String] = None) extends CBVValue

case class VariableValue(n: String, maybeContextName: Option[String] = None) extends CBVValue
//case class ListValue(values: Seq[Value], maybeContextName: Option[String] = None) extends CBVValue

