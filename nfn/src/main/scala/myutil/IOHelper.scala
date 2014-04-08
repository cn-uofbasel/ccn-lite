package myutil

import java.io.{PrintWriter, StringWriter}

/**
 * Created by basil on 08/04/14.
 */
object IOHelper {

  def exceptionToString(error: Throwable): String = {
    val sw = new StringWriter()
    val pw = new PrintWriter(sw)
    error.printStackTrace(pw)
    sw.toString
  }
}
