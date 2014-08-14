package bytecode

import org.scalatest.{Matchers, FlatSpec}
import scala.util.{Failure, Success}

case class TestClass() {
  def fun = println("TestClass.fun")
}

class BytecodeLoaderTest extends FlatSpec with Matchers {

//  "The bytecode of a compiled class" should "be loaded from its bytecode stored on the filesystem and be an instance of the corresponding class" in {
//    import BytecodeLoader._
//    val classDir = classToDirname(new TestClass())
//    loadClass[TestClass](classDir, TestClass().getClass.getCanonicalName).get should be (an[TestClass])
//  }
}
