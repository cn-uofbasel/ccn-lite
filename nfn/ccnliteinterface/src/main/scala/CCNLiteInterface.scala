class CCNLiteInterface {
  System.loadLibrary("CCNLiteInterface")

  @native private def mkBinaryInterest(name: String): Array[Byte]
  @native private def mkBinaryContent(name: String, data: Array[Byte]): Array[Byte]
  @native private def ccnbToXml(binaryPacket: Array[Byte]): String
}

object CCNLiteInterface {
  private val ccnIf = new CCNLiteInterface()

  def mkBinaryInterest(interest: Interest): Array[Byte] = {
    ccnIf.mkBinaryInterest(interest.name)
  }
  def mkBinaryContent(content: Content): Array[Byte] = {
    ccnIf.mkBinaryContent(content.name, content.data)
  }
  def ccnbToXml(binaryPacket: Array[Byte]): String = {
    ccnIf.ccnbToXml(binaryPacket)
  }

  def main(args: Array[String]) = {
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
}
