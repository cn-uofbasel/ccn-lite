package ccn

import javax.xml.bind.DatatypeConverter

import ccn.packet._
import com.typesafe.scalalogging.slf4j.Logging
import org.xml.sax.SAXParseException

import scala.xml._


object NFNCCNLiteParser extends Logging {

  def parseCCNPacket(xmlString: String): Option[CCNPacket] = {
    parsePacket(xmlString) match {
      case Some(ccnPacket:CCNPacket) => Some(ccnPacket)
      case _ =>  None
    }
  }

  def decodeBase64(data: String): Array[Byte] = DatatypeConverter.parseBase64Binary(data)
  def encodeBase64(bytes: Array[Byte]): String = DatatypeConverter.printBase64Binary(bytes)


  def parsePacket(xmlString: String):Option[Packet] = {
    def parseData(elem: Node): String = {
      val data = elem \ "data"

      val nameSize = (data \ "@size").text.toInt
      val encoding = (data \ "@dt").text
      val nameData = data.text.trim

      encoding match {
        case "string" =>
          nameData
        case "binary.base64" =>
          new String(decodeBase64(nameData))
        case _ => throw new Exception(s"parseData() does not support data of type: '$encoding'")
      }
    }

    def parseComponentsCCNB(elem: Elem):Seq[String] = {
      val components = elem \ "name" \ "component"

      components.map { parseData }
    }

    def parseComponentsNDNTLV(elem: Elem):Seq[String] = {
      val components = elem \ "Name" \ "NameComponent"

      components.map { parseData }
    }

    def parseContentData(elem: Elem): Array[Byte] = {
      val contents = elem \ "content"
      assert(contents.size == 1, "content should only contain one node with content")
      parseData(contents.head).getBytes
    }

    val cleanedXmlString = xmlString.trim.replace("&", "&amp;")

    try {
      val xml: Elem = scala.xml.XML.loadString(cleanedXmlString)
      Some(
        xml match {
          case interest @ <interest>{_*}</interest>=> {
            val nameComponents = parseComponentsCCNB(interest)
            Interest(nameComponents :_*)
          }
          case content @ <contentobj>{_*}</contentobj> => {
            val nameComponents = parseComponentsCCNB(content)
            val contentData = parseContentData(content)
            if(new String(contentData).startsWith(":NACK")) {
              NAck(CCNName(nameComponents :_*))
            } else {
              Content(contentData, nameComponents :_*)
            }
          }
          case interest @ <Interest>{_*}</Interest>=> {
            val nameComponents = parseComponentsNDNTLV(interest)
            Interest(nameComponents :_*)
          }
          case content @ <Data>{_*}</Data> => {
            val nameComponents = parseComponentsNDNTLV(content)
            val contentData = parseContentData(content)
            if(new String(contentData).startsWith(":NACK")) {
              NAck(CCNName(nameComponents :_*))
            } else {
              Content(contentData, nameComponents :_*)
            }
          }
          case _ => throw new Exception("XML parser cannot parse:\n" + xml)
        }
      )
    } catch {
      case e:SAXParseException => {
        logger.error(s"SAXParseException (not printed) when parsing the xml message of ccnbToXml string:\n$cleanedXmlString")
        None
      }
    }
  }
}

