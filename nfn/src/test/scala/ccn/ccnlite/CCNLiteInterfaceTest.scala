package ccn.ccnlite

import ccnliteinterface.CCNLiteInterface
import org.scalatest.{GivenWhenThen, Matchers, FlatSpec}
import ccn.packet._
import network.NFNCommunication


/**
* Created by basil on 05/03/14.
*/
class CCNLiteInterfaceTest extends FlatSpec with Matchers with GivenWhenThen {
  val ccnIf = new CCNLiteInterface()
  val interest = Interest(Seq("/name/interest"))

  s"Interest $interest" should "be converted to ccnb back to xml into interest object" in {
    Given("cnnb for name")
    val ccnbInterest = ccnIf.mkBinaryInterest(interest.nameComponents.toArray)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbInterest)
    Then("xml parsed to interest")
    val resultInterest = NFNCommunication.parseCCNPacket(xmlUnparsed)
    resultInterest.get should be (a [Interest])
    resultInterest.get.name should be (Seq("name", "interest"))
  }

  val content:Content = Content(Seq("/name/content"), "testcontent".getBytes)

  s"Content $content" should "be converted to ccnb back to xml into content object" in {
    Given("cnnb for name and content")
    val ccnbContent = ccnIf.mkBinaryContent(content.nameComponents.toArray, content.data)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbContent)
    Then("xml parsed to content")
    val resultContent = NFNCommunication.parseCCNPacket(xmlUnparsed)
    resultContent should be (a [Content])
    resultContent.get.name should be (Seq("name", "content"))
    resultContent.get.asInstanceOf[Content].data should be ("testcontent".getBytes)
  }
}
