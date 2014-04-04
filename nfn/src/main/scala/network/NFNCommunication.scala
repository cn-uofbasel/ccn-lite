package network

import scala.xml._

import javax.xml.bind.DatatypeConverter
import org.xml.sax.SAXParseException

import com.typesafe.scalalogging.slf4j.Logging

import ccn.packet.{Packet, Content, Interest}


object NFNCommunication extends Logging {

  def parseXml(xmlString: String):Option[Packet] = {
    def parseData(elem: Node): String = {
      val data = elem \ "data"

      val nameSize = (data \ "@size").text.toInt
      val encoding = (data \ "@dt").text
      val nameData = data.text.trim

      encoding match {
        case "string" =>
          nameData
        case "binary.base64" =>
          new String(DatatypeConverter.parseBase64Binary(nameData))
        case _ => throw new Exception(s"parseData() does not support data of type: '$encoding'")
      }
    }

    def parseComponents(elem: Elem):Seq[String] = {
      val components = elem \ "name" \ "component"

      components.map { parseData }
    }

    def parseContent(elem: Elem): Array[Byte] = {
      val contents = elem \ "content"
      assert(contents.size == 1, "content should only contain one node with content")
      parseData(contents.head).getBytes
    }

    val cleanedXmlString = xmlString.trim.replace("&", "&amp;")

    try {
      val xml: Elem = scala.xml.XML.loadString(cleanedXmlString)
      Some(
        xml match {
          case interest @ <interest>{_*}</interest> => {
            val nameComponents = parseComponents(interest)
            Interest(nameComponents)
          }
          case content @ <contentobj>{_*}</contentobj> => {
            val nameComponents = parseComponents(content)
            val contentData = parseContent(content)
            Content(nameComponents, contentData)
          }
          case _ => throw new Exception("XML parser cannot parse:\n" + xml)
        }
      )
    } catch {
      case e:SAXParseException => None
    }
  }
}

