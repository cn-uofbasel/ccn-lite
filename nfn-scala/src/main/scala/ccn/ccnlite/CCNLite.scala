package ccn.ccnlite

import ccn.NFNCCNLiteParser
import ccnliteinterface.CCNLiteInterface
import ccn.packet._
import java.io.{FileReader, FileOutputStream, File}
import com.typesafe.scalalogging.slf4j.Logging
import myutil.IOHelper


/**
 * Wrapper for the [[CCNLiteInterface]]
 */
object CCNLite extends Logging {
  val ccnIf = new CCNLiteInterface()

  def ccnbToXml(ccnbData: Array[Byte]): String = {
    // This synchronized is required because currently ccnbToXml writes to the local file c_xml.txt
    // This results in issues when concurrenlty writing/creating/deleting the same file (filelock)
    // Fix the implementation of Java_ccnliteinterface_CCNLiteInterface_ccnbToXml in the ccnliteinterface and remove synchronized
    this.synchronized {
      ccnIf.ccnbToXml(ccnbData)
    }
  }

  def mkBinaryContent(content: Content): Array[Byte] = {
    val name = content.name.cmps.toArray
    val data = content.data
    ccnIf.mkBinaryContent(name, data)
  }

  def mkBinaryContent(name: Array[String], data: Array[Byte]): Array[Byte] = {
    ccnIf.mkBinaryContent(name, data)
  }

  def mkBinaryInterest(interest: Interest): Array[Byte] = {
    ccnIf.mkBinaryInterest(interest.name.cmps.toArray)
  }

  def mkBinaryPacket(packet: CCNPacket): Array[Byte] = {
    packet match {
      case i: Interest => mkBinaryInterest(i)
      case c: Content => mkBinaryContent(c)
      case n: NAck => mkBinaryContent(n.toContent)
    }
  }

  def mkBinaryInterest(name: Array[String]): Array[Byte] = {
    ccnIf.mkBinaryInterest(name)
  }


  def mkAddToCacheInterest(content: Content): Array[Byte] = {
    val binaryContent = mkBinaryContent(content)

    val servLibDir = new File("./service-library")
    if(!servLibDir.exists) {
      servLibDir.mkdir()
    }
    val filename = s"./service-library/${content.name.hashCode}-${System.nanoTime}.ccnb"
    val file = new File(filename)

    // Just to be sure, if the file already exists, wait quickly and try again
    if (file.exists) {
      logger.warn(s"Temporary file already existed, this should never happen!")
      Thread.sleep(1)
      mkAddToCacheInterest(content)
    } else {
      file.createNewFile
      val out = new FileOutputStream(file)
      try {
        out.write(binaryContent)
      } finally {
        if (out != null) out.close
      }
      val absoluteFilename = file.getCanonicalPath
      val binaryInterest = ccnIf.mkAddToCacheInterest(absoluteFilename)

      file.delete
      binaryInterest
    }
  }

  def base64CCNBToPacket(base64ccnb: String): Option[CCNPacket] = {
    val xml = CCNLite.ccnbToXml(NFNCCNLiteParser.decodeBase64(base64ccnb))
    val pkt = NFNCCNLiteParser.parseCCNPacket(xml)
    pkt
  }

  private def mkAddToCacheInterest(ccnbAbsoluteFilename: String): Array[Byte] = {
    ccnIf.mkAddToCacheInterest(ccnbAbsoluteFilename)
  }
}
