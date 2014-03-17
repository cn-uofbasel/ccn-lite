package secmachine.machine.CallByName

import secmachine.machine._


case class CBNConfiguration(stack: List[Value], env:List[Value], code:List[Instruction]) extends Configuration {
  override def toString:String = {
    val sb = new StringBuilder()
    sb ++=   "stack: " ++ stack.mkString("(", ", ", ")")
    sb ++= "\nenv  : " ++ env.mkString("(", ", ", ")")
    sb ++= "\ncode : " ++ code.mkString("(", ", ", ")")
    sb.toString()
  }

  override def isTransformable: Boolean = !code.isEmpty
}
