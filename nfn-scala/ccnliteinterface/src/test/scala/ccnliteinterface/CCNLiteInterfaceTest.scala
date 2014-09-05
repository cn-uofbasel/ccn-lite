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

    def shortenStringToN(str: String, n: Int) =
      if(str.size > n) {
        str.take(n) + s" ... (size=${str.size})"
      } else {
        str
      }

    def nameToString(name: Array[String]): String = {
      name.map { cmp =>
        shortenStringToN(cmp, 20)
      } mkString("[ ", " | ", " ]")
    }

    def stringContainsBase64EncodedString(base: String, matcher: String): Boolean = {
      val base64Enc = new BASE64Encoder()
      val b64Cmp = base64Enc.encode(matcher.getBytes)
      base.contains(b64Cmp)
    }

    def testCCNLiteInterfaceCreateInterestWithName(name: Array[String]) = {
      s"CCNLiteInterface $ifType with wire format $wireFormat for name ${nameToString(name)}" should s"convert to an interest msg, back to xml and contain the name" in {
        Given(s" $wireFormat for name")
        val wfInterest = ccnIf.mkBinaryInterest(name)
        When("parsed to xml string")
        val xml = ccnIf.ccnbToXml(wfInterest)
        Then(s"xml parsed to interest")
        name.foreach { cmp =>
          stringContainsBase64EncodedString(xml, cmp) shouldBe true
        }
      }
    }
    def testCCNLiteInterfaceCreateContent(name: Array[String], dataString: String) = {

      s"CCNLiteInterface $ifType with wire format $wireFormat for name ${nameToString(name)} with data ${shortenStringToN(dataString, 20)}" should "convert to a content msg, back to xml and contain the name and data" in {
        Given(s"$wireFormat for name and data")
        val wfContent = ccnIf.mkBinaryContent(name, dataString.getBytes)
        When("parsed to xml string")
        val xml = ccnIf.ccnbToXml(wfContent)
        Then(s"xml: parsed to content")
        name.foreach { cmp =>
          stringContainsBase64EncodedString(xml, cmp) shouldBe true
        }
        stringContainsBase64EncodedString(xml, dataString)
      }
    }




    val names: List[Array[String]] = {
      def nameCmpOfSizeN(n: Int) = {
        val cmpDescription = s"cmp${n}chars"

        if (n > cmpDescription.size) {
          cmpDescription + "a" * (n - cmpDescription.size)
        } else {
          cmpDescription.take(n)
        }
      }

      List(
        Array("test", "name"),
        Array[String](),
        Array(nameCmpOfSizeN(57)),
        Array(nameCmpOfSizeN(57), nameCmpOfSizeN(57))
      )
    }

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
