class CCNLiteInterface {
//  System.setProperty("java.library.path", "/Users/basil/Dropbox/uni/master_thesis/code/ccnliteinterface/src/main/c/ccn-lite-bridge")

  System.loadLibrary("CCNLiteInterface")

  @native def mkBinaryInterest(name: String): Array[Byte]
  @native def mkBinaryContent(name: String, data: Array[Byte]): Array[Byte]
  @native def ccnbToXml(binaryInterest: Array[Byte]): String
}

object CCNLiteInterface extends App {

  val ccnIf = new CCNLiteInterface()

  val ccnbInterest: Array[Byte] = ccnIf.mkBinaryInterest("/contentname/interest")
  val xmlInterest:String = ccnIf.ccnbToXml(ccnbInterest)
  println(s"Interest:\n$xmlInterest")

  val data = "testdata".getBytes()
  val ccnbContent: Array[Byte] = ccnIf.mkBinaryContent("/contentname/content", data)
  val xmlContent:String = ccnIf.ccnbToXml(ccnbContent)
  println(s"Content:\n$xmlContent")

//  println(s"Last: ${xmlContent.substring(xmlContent.size - 10, xmlContent.size)}")
}
