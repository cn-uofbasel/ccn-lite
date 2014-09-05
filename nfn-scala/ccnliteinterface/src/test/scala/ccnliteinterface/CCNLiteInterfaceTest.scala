package ccnliteinterface

import java.io.{FileWriter, File}

import org.scalatest.{GivenWhenThen, Matchers, FlatSpec}
import sun.misc.BASE64Encoder

class CCNLiteInterfaceTest extends FlatSpec with Matchers with GivenWhenThen {

  testCCNLiteInterface(CCNBWireFormat(), CCNLiteCliInterface())
  testCCNLiteInterface(CCNBWireFormat(), CCNLiteJniInterface())

  testCCNLiteInterface(NDNTLVWireFormat(), CCNLiteCliInterface())
//  testCCNLiteInterface(NDNTLVWireFormat(), CCNLiteJniInterface())



  def testCCNLiteInterface(wireFormat: CCNLiteWireFormat, ifType: CCNLiteInterfaceType) = {

    val ccnIf = CCNLiteInterface.createCCNLiteInterface(wireFormat, ifType)

    def stringContainsBase64EncodedString(base: String, matcher: String): Boolean = {
      val base64Enc = new BASE64Encoder()
      val b64Cmp = base64Enc.encode(matcher.getBytes)
      base.contains(b64Cmp)
    }

    def testCCNLiteInterfaceCreateInterestWithName(name: Array[String]) = {
      s"CCNLiteInterface $ifType with wire format $wireFormat for name ${name.mkString("[", ", ", "]")}" should s"convert to an interest msg, back to xml and contain the name" in {
        Given(s" $wireFormat for name")
        val wfInterest = ccnIf.mkBinaryInterest(name)
        When("parsed to xml string")
        val xml = ccnIf.ccnbToXml(wfInterest)
        Then(s"xml:\n$xml\nparsed to interest")
        name.foreach { cmp =>
          stringContainsBase64EncodedString(xml, cmp) shouldBe true
        }
      }
    }
    def testCCNLiteInterfaceCreateContent(name: Array[String], dataString: String) = {
      s"CCNLiteInterface $ifType with wire format $wireFormat for name ${name.mkString("[", ", ", "]")} with data $dataString" should "convert to a content msg, back to xml and contain the name and data" in {
        Given(s"$wireFormat for name and data")
        val wfContent = ccnIf.mkBinaryContent(name, dataString.getBytes)
        When("parsed to xml string")
        val xml = ccnIf.ccnbToXml(wfContent)
        Then(s"xml:\n$xml\nparsed to content")
        name.foreach { cmp =>
          stringContainsBase64EncodedString(xml, cmp) shouldBe true
        }
        stringContainsBase64EncodedString(xml, dataString)
      }
    }

    val names: List[Array[String]] = List(
      Array("test", "name"),
      Array[String](),
      Array("a"*57)
    )

    val dataStrings: List[String] = List(
      "testdata",
      "",
      "a"*8000
    )

    names foreach testCCNLiteInterfaceCreateInterestWithName

    for {
      name <- names
      dataString <- dataStrings
    } yield {
      testCCNLiteInterfaceCreateContent(name, dataString)
    }

    s"CCNLiteInterface $ifType with wire format $wireFormat" should s"create an addToCache request message containing the content of a testfile" in {

      val testfile = new File("./testfile" + System.currentTimeMillis())
      val fw = new FileWriter(testfile)
      try{ fw.write("testdata")} finally {fw.close()}

      val msg = ccnIf.mkAddToCacheInterest(testfile.getCanonicalPath)

      new String(msg).contains("testdata") shouldBe true

      testfile.delete
    }
  }
}
