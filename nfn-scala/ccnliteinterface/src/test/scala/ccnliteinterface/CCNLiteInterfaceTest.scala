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

    val name = Array("test", "name")

    def stringContainsBase64EncodedString(base: String, matcher: String): Boolean = {
      val base64Enc = new BASE64Encoder()
      val b64Cmp = base64Enc.encode(matcher.getBytes)
      base.contains(b64Cmp)
    }

    s"CCNLiteInterface $ifType with wire format $wireFormat" should "convert a name to an interest msg, back to xml and contain the name" in {
      Given(s" $wireFormat for name")
      val wfInterest = ccnIf.mkBinaryInterest(name)
      When("parsed to xml string")
      val xml = ccnIf.ccnbToXml(wfInterest)
      Then(s"xml:\n$xml\nparsed to interest")
      name.foreach { cmp =>
        stringContainsBase64EncodedString(xml, cmp) shouldBe true
      }
    }

    s"CCNLiteInterface $ifType with wire format $wireFormat" should "convert a name and data to an content msg, back to xml and contain the name and data" in {
      val dataString = "testdata"
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
