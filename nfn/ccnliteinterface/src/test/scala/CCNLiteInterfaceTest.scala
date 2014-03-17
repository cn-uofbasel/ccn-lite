import org.scalatest.{GivenWhenThen, Matchers, FlatSpec}


/**
 * Created by basil on 05/03/14.
 */
class CCNLiteInterfaceTest extends FlatSpec with Matchers with GivenWhenThen {
  val ccnIf = new CCNLiteInterface()

  "Interest for name '/name/interest'" should "be converted to ccnb back to xml into interest object" in {
    Given("cnnb for name")
    val ccnbInterest = ccnIf.mkBinaryInterest("/name/interest")
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbInterest)
    Then("xml parsed to interest")
    val interest = NFNCommunication.parseXml(xmlUnparsed)
    interest should be (a [Interest])
    interest.name should be (Seq("name", "interest"))
  }

  "Content 'testcontent' for name '/name/content'" should "be converted to ccnb back to xml into content object" in {
    Given("cnnb for name and content")
    val ccnbContent = ccnIf.mkBinaryContent("/name/content", "testcontent".getBytes)
    When("parsed to xml string")
    val xmlUnparsed = ccnIf.ccnbToXml(ccnbContent)
    Then("xml parsed to content")
    val content = NFNCommunication.parseXml(xmlUnparsed)
    content should be (a [Content])
    content.name should be (Seq("name", "content"))
    content.asInstanceOf[Content].data should be ("testcontent".getBytes)
  }
}
