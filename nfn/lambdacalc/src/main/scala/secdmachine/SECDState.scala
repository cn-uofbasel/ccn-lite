package secdmachine

import scala.collection.mutable

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 15:23
 * To change this template use File | Settings | File Templates.
 */

class SECDState {
  var stack: List[CellValue] = _
  var env:List[List[CellValue]] = _
  var code:List[Instruction] = _
  var dump:List[Instruction] = _

  override def toString():String = {
    val sb = new StringBuilder()
    sb ++=   "stack: " ++ stack.mkString("(", ", ", ")")
    sb ++= "\nenv  : " ++ Helper.envToString(env)
    sb ++= "\ncode : " ++ code.mkString("(", ", ", ")")
    sb ++= "\ndump : " ++ dump.mkString("(", ", ", ")")

    sb.toString()
  }
}
