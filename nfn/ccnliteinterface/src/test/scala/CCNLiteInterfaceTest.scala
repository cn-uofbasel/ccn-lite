import org.scalatest.{GivenWhenThen, Matchers, FlatSpec}


/**
 * Created by basil on 05/03/14.
 */
class CCNLiteInterfaceTest extends FlatSpec with Matchers with GivenWhenThen {
  val ccnIf = CCNLiteInterface
  val interest = Interest("/name/interest")

  s"Interest $interest" should "be converted to ccnb back to xml into interest object" in {
    Given("cnnb for name")
    val ccnbInterest = ccnIf.mkBinaryInterest(interest)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbInterest)
    Then("xml parsed to interest")
    val resultInterest = NFNCommunication.parseXml(xmlUnparsed)
    resultInterest should be (a [Interest])
    resultInterest.name should be (Seq("name", "interest"))
  }

  val content:Content = Content("/name/content", "testcontent".getBytes)

  s"Content $content" should "be converted to ccnb back to xml into content object" in {
    Given("cnnb for name and content")
    val ccnbContent = ccnIf.mkBinaryContent(content)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbContent)
    Then("xml parsed to content")
    val resultContent = NFNCommunication.parseXml(xmlUnparsed)
    resultContent should be (a [Content])
    resultContent.name should be (Seq("name", "content"))
    resultContent.asInstanceOf[Content].data should be ("testcontent".getBytes)
  }
}
