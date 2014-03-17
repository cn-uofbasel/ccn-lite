package lambdasecd

import scala.collection.immutable.ListMap

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 27/11/13
 * Time: 15:29
 * To change this template use File | Settings | File Templates.
 */
object Helper {
  implicit def envAsString(env: List[ListMap[String, Expr]]):String = {
    val sb = new StringBuilder()
    env.foreach( sb ++= _.mkString("[", ", ", "]"))
    sb.toString()
  }
}
