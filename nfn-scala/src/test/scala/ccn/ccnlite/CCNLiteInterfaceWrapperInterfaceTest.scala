package ccn.ccnlite

import java.io.{FileWriter, File}

import ccn.NFNCCNLiteParser
import ccnliteinterface._
import org.scalatest.{GivenWhenThen, Matchers, FlatSpec}
import ccn.packet._


/**
* Created by basil on 05/03/14.
*/
class CCNLiteInterfaceWrapperInterfaceTest extends FlatSpec with Matchers with GivenWhenThen {

  testCCNLiteInterfaceWrapper(CCNBWireFormat(), CCNLiteJniInterface())
  def testCCNLiteInterfaceWrapper(wireFormat: CCNBWireFormat, ifType: CCNLiteInterfaceType) = {
    val ccnIf = CCNLiteInterface.createCCNLiteInterface(CCNBWireFormat(), CCNLiteJniInterface())

    val interest = Interest("name", "interest")

    s"CCNLiteInterface of type $ifType with wire format $wireFormat with interest $interest" should "be converted to ccnb back to xml into an interest object" in {
      val ccnbInterest = ccnIf.mkBinaryInterest(interest.name.cmps.toArray)
      val xmlUnparsed = ccnIf.ccnbToXml(ccnbInterest)
      val resultInterest = NFNCCNLiteParser.parseCCNPacket(xmlUnparsed)
      resultInterest.get should be (a [Interest])
      resultInterest.get.name.cmps should be (Seq("name", "interest"))
    }

    val content:Content = Content("testcontent".getBytes, "name", "content")

    s"CCNLiteInterface of type $ifType with wire format $wireFormat with content $content" should "be converted to ccnb back to xml into content object" in {
      val ccnbContent = ccnIf.mkBinaryContent(content.name.cmps.toArray, content.data)
      val xmlUnparsed = ccnIf.ccnbToXml(ccnbContent)
      val resultContent = NFNCCNLiteParser.parseCCNPacket(xmlUnparsed)
      resultContent.get should be (a [Content])
      resultContent.get.name.cmps should be (Seq("name", "content"))
      resultContent.get.asInstanceOf[Content].data should be ("testcontent".getBytes)
    }
    s"Content $content" should "be converted to ccnb for an addToCache requeest" in {

      val testfile = new File("./testfile" + System.currentTimeMillis())
      val fw = new FileWriter(testfile)
      try { fw.write("testdata")} finally {fw.close()}

      val ccnAddToCacheReq = ccnIf.mkAddToCacheInterest(testfile.getCanonicalPath)
      new String(ccnAddToCacheReq).contains("testdata") shouldBe true

      testfile.delete()
    }
  }

}
