package secmachine

import scala.io.Source
import scala.util.{Try,Success,Failure}

object Test extends App {

  def readTextFile(filename: String): Try[List[String]] = {
    Try(Source.fromFile(filename).getLines.toList)
  }

  val filename = "/etc/pass"
  readTextFile(filename) match {
    case Success(lines) => lines.foreach(println)
    case Failure(f) => println(f)
  }

}
