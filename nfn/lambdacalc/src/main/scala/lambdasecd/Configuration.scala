package lambdasecd

import scala.collection.immutable.ListMap

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 27/11/13
 * Time: 14:53
 * To change this template use File | Settings | File Templates.
 */
// TODO Env is not working correctly yet, see p229
case class Configuration(S:List[Expr], E:List[ListMap[String, Expr]], C:List[Expr], D:Configuration) {
  override def toString() = {
    val sb = new StringBuilder()
    sb ++= "stack: "
    sb ++= S.mkString("{", ", ", "}")
    sb ++= "\nenv: "
    sb ++= Helper.envAsString(E)
    sb ++="\ncontrol: "
    sb ++= C.mkString("{", ", ", "}")
    sb ++= "\ndump: "
    sb ++= (if(D != null) D.toString() else "")
    sb.toString()
  }
}
