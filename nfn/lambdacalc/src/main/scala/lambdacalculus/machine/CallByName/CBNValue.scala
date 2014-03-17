package lambdacalculus.machine.CallByName

import lambdacalculus.machine._

trait CBNValue extends Value
case class ClosureThunk(c: List[Instruction], e: List[Value]) extends CBNValue
