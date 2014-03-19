
class CCNLiteInterface {
  System.loadLibrary("CCNLiteInterface")

  @native def mkBinaryInterest(nameCmps: Array[String]): Array[Byte]
  @native def mkBinaryContent(name: Array[String], data: Array[Byte]): Array[Byte]
  @native def ccnbToXml(binaryPacket: Array[Byte]): String
}

object CCNLiteInterface {
  private val ccnIf = new CCNLiteInterface()

//  def mkBinaryInterest(interest: Interest): Array[Byte] = {
//
//    ccnIf.mkBinaryInterest(interest.nameComponents)
//  }
//  def mkBinaryContent(content: Content): Array[Byte] = {
//    ccnIf.mkBinaryContent(content.name, content.data)
//  }
//  def ccnbToXml(binaryPacket: Array[Byte]): String = {
//    ccnIf.ccnbToXml(binaryPacket)
//  }

  def main(args: Array[String]) = {
    val ccnIf = new CCNLiteInterface()

    val ccnbInterest: Array[Byte] = ccnIf.mkBinaryInterest(Array("/contentname/interest", "NFN"))
    val xmlInterest:String = ccnIf.ccnbToXml(ccnbInterest)
    println(s"Interest:\n$xmlInterest")

    val data = "testdata".getBytes()
    val ccnbContent: Array[Byte] = ccnIf.mkBinaryContent(Array("/contentname/content", "NFN"), data)
    val xmlContent:String = ccnIf.ccnbToXml(ccnbContent)
    println(s"Content:\n$xmlContent")

    //  println(s"Last: ${xmlContent.substring(xmlContent.size - 10, xmlContent.size)}")
  }
}


