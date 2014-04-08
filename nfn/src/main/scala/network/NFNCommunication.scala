package network

import scala.xml._

import javax.xml.bind.DatatypeConverter
import org.xml.sax.SAXParseException

import com.typesafe.scalalogging.slf4j.Logging

import ccn.packet._


object NFNCommunication extends Logging {


  def parseCCNPacket(xmlString: String): Option[CCNPacket] = {
    parsePacket(xmlString) match {
      case Some(ccnPacket:CCNPacket) => Some(ccnPacket)
      case _ => None
    }
  }

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
          new String(DatatypeConverter.parseBase64Binary(nameData))
        case _ => throw new Exception(s"parseData() does not support data of type: '$encoding'")
      }
    }

    def parseComponents(elem: Elem):Seq[String] = {
      val components = elem \ "name" \ "component"

      components.map { parseData }
    }

    def parseContentData(elem: Elem): Array[Byte] = {
      val contents = elem \ "content"
      assert(contents.size == 1, "content should only contain one node with content")
      parseData(contents.head).getBytes
    }


    // TODO fix plz!
    // This is a hack to avoid parsing addToCache packets.
    // The main issue is that these packets have a ccnlite specific management module format
    if(xmlString.hashCode == -1007970307) {
      Some(AddToCache())
    }
    else {
      val cleanedXmlString = xmlString.trim.replace("&", "&amp;")

      try {
        val xml: Elem = scala.xml.XML.loadString(cleanedXmlString)
        Some(
          xml match {
            case interest @ <interest>{_*}</interest> => {
              val nameComponents = parseComponents(interest)
              Interest(nameComponents)
            }
  //  This is the start of parsing management packets.
  // Something is wrong in the conversion to xml and the 'unkown' tag is not closed propery.
  // And it obviously should not be 'unkown' anyway.
  //          case fragment @ <contentobj><fragmentation>{_*}</fragmentation></contentobj> => {
  //            val any = fragment \ "any"
  //            assert(any.size == 1, "When parsing the fragmentation content there was not just one any element")
  //            val anyData = parseData(any.head)
  //            // TODO oops, fix this in native code
  //            val unkown = fragment \ "unkown"
  //            assert(unkown.size == 1, "When parsing the ?unkown? content there was not just one any element")
  //            val unkownData = parseData(unkown.head)
  //            println(s"ANY: ${new String(anyData)}")
  //            println(s"UNKOWN: ${new String(unkownData)}")
  //            ???
  //          }
            case content @ <contentobj>{_*}</contentobj> => {
              val nameComponents = parseComponents(content)
              val contentData = parseContentData(content)
              Content(nameComponents, contentData)
            }
            case _ => throw new Exception("XML parser cannot parse:\n" + xml)
          }
        )
      } catch {
        case e:SAXParseException => {

          if(xmlString.hashCode == -1007970307) println("AddContentAck")
          else println(s"Ooops")
          None
        }
      }
    }
  }
}

