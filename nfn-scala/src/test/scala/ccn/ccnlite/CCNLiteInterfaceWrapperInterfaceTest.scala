package ccn.ccnlite

import java.io.{FileWriter, File}

import ccn.NFNCCNLiteParser
import ccnliteinterface.CCNLiteInterface
import org.scalatest.{GivenWhenThen, Matchers, FlatSpec}
import ccn.packet._


/**
* Created by basil on 05/03/14.
*/
class CCNLiteInterfaceWrapperInterfaceTest extends FlatSpec with Matchers with GivenWhenThen {
  val ccnIf = CCNLiteInterfaceWrapper.createCCNLiteInterface(CCNBWireFormat(), CCNLiteJniInterface())
  val interest = Interest("name", "interest")

  s"Interest $interest" should "be converted to ccnb back to xml into interest object" in {
    Given("cnnb for name")
    val ccnbInterest = ccnIf.mkBinaryInterest(interest.name.cmps.toArray)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbInterest)
    Then("xml parsed to interest")
    val resultInterest = NFNCCNLiteParser.parseCCNPacket(xmlUnparsed)
    resultInterest.get should be (a [Interest])
    resultInterest.get.name.cmps should be (Seq("name", "interest"))
  }

  val content:Content = Content("testcontent".getBytes, "name", "content")

  s"Content $content" should "be converted to ccnb back to xml into content object" in {
    Given("cnnb for name and content")
    val ccnbContent = ccnIf.mkBinaryContent(content.name.cmps.toArray, content.data)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbContent)
    Then("xml parsed to content")
    val resultContent = NFNCCNLiteParser.parseCCNPacket(xmlUnparsed)
    resultContent.get should be (a [Content])
    resultContent.get.name.cmps should be (Seq("name", "content"))
    resultContent.get.asInstanceOf[Content].data should be ("testcontent".getBytes)
  }

//  s"Content $content" should "be converted to ccnb for an addToCache requeest" in {
//    val f = new File("./testfile")
//    val fw = new FileWriter(f)
//    fw.write("test")
//    ccnIf.mkAddToCacheInterest()
//    val ccnAddToCacheReq = ccnIf.mkAddToCacheInterest(f.getCanonicalPath)
//  }
}
