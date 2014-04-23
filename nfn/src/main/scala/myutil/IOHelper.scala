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

  /**
   * Usage:
   * import java.io._
   * val data = Array("Five","strings","in","a","file!")
   * printToFile(new File("example.txt"))(p => {
   *   data.foreach(p.println)
   * })
   * @param f
   * @param op
   * @return
   */
  def printToFile(f: java.io.File)(op: java.io.PrintWriter => Unit) {
    val p = new java.io.PrintWriter(f)
    try { op(p) } finally { p.close() }
  }

  def printToFile(f: java.io.File, data: String) {
    printToFile(f)(_.println(data))
  }
}
