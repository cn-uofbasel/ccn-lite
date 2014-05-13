package ccnliteinterface

/**
* To call these functions the natively compiled library must exist and added to the classpath.
* Call ./make.sh in the folder ./ccnliteinterface/src/main/c/ccn-lite-bridge (currently only works for OSX)
* and then add:
* -Djava.library.path=./ccnliteinterface/src/main/c/ccn-lite-bridge
* when executing the final code.
*
* These functions are somewhat fragile and might result in a JVM crash. Check stderr for "<native>..." messages.
*/
class CCNLiteInterface {
  System.loadLibrary("CCNLiteInterface")

  /**
   * Native call to create a binary interest for name components
   * @param nameCmps Each element of this array is a seperate component for the interest name
   * @return Binary data of interest in ccnb format
   */
  @native def mkBinaryInterest(nameCmps: Array[String]): Array[Byte]

  /**
   * Native call to create a binary content object for name compoents and given data
   * @param name Each elemet of this array is a separate component for the content name
   * @param data Binary data of the content object
   * @return Binary data of the content in ccnb format
   */
  @native def mkBinaryContent(name: Array[String], data: Array[Byte]): Array[Byte]

  /**
   * Creates an xml representation for ccnb formated data.
   * attribute dt for the data element represents the encoding of the data. Currently "string" or "base64" is supported.
   * String is fragile because it can contain unescaped characters.
   *
   * Interest(["cmp1", "cmp2"]:
   * <interest>
   * <name>
   *   <component>
   *     <data size="4" dt="string">
   *         cmp1
   *     </data>
   *   </component>
   *   <component>
   *     <data size="4" dt="string">
   *         cmp2
   *     </data>
   *   </component>
   * </name>
   * <scope>
   *   <data size="0" dt="string">
   *
   *   </data>
   * </scope>
   * <nonce>
   *   <data size="4" dt="string">
   *       '%0d0S
   *   </data>
   * </nonce>
   * </interest>
   *
   * Content(["cmp1", "cmp2"], "content"):
   * <contentobj>
   *   <name>
   *     <component>
   *       <data size="4" dt="string">
   *           "cmp1"
   *       </data>
   *     </component>
   *     <component>
   *       <data size="4" dt="string">
   *           "cmp2"
   *       </data>
   *     </component>
   *   </name>
   *   <timestamp>
   *     <data size="6" dt="string">
   *         S0%0d'%07/
   *     </data>
   *   </timestamp>
   *   <signedinfo>
   *     <publisherPubKeyDigest>
   *       <data size="0" dt="string">

   *       </data>
   *     </publisherPubKeyDigest>
   *   </signedinfo>
   *   <content>
   *     <data size="8" dt="string">
   *         content
   *     </data>
   *   </content>
   * <contentobj>
   *
   * @param binaryPacket Data in ccnb format
   * @return A string containing the xml representation of the ccnb data
   */
  @native def ccnbToXml(binaryPacket: Array[Byte]): String

  /**
   * Creates an ccnb format interest to add a content object to the localAbstractMachine ccn cache.
   * The content object itself must formated in ccnb and stored in a file.
   * @param ccnbAbsoluteFilename Filename of the file containing the content object formated in ccnb
   * @return Binary data of the interest in ccnb format
   */
  @native def mkAddToCacheInterest(ccnbAbsoluteFilename: String): Array[Byte]


}

object CCNLiteInterface {

  private val ccnIf = new CCNLiteInterface()

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


