import scala.concurrent._

import ExecutionContext.Implicits.global

import java.io.File
import java.util.Date
import scala.util.{Failure, Success}
import scala.xml.Elem

trait Packet {
  def name: Seq[String]
}

case class Interest(name: Seq[String]) extends Packet
case class Content(name: Seq[String], data: Array[Byte]) extends Packet

object NFNCommunication {
  def parseXml(xmlString: String):Packet = {
    def parseComponents(elem: Elem):Seq[String] = {
      val components = elem \ "name" \ "component"

      components.map { component =>
        val data = component \ "data"

        val nameSize = (data \ "@size").text.toInt
        val nameData = data.text.trim

        assert(nameData.size == nameSize, s"Parsed name '$nameData' has not its actual size $nameSize")

        nameData
      }
    }

    def parseContent(elem: Elem): Array[Byte] = {
      val content = elem \ "content" \ "data"

      val contentSize = (content \ "@size").text.toInt
      val contentData = content.text.trim



      println(s"Parsed data (size = $contentSize): '$contentData'")
      assert(contentData.size == contentSize, s"Parsed data '${new String(contentData)}' has not its actual size $contentSize")

      contentData.getBytes
    }

//    val xmlStringTrimmed = xmlString.trim
//
//    val endOfFirstTag = xmlStringTrimmed.indexOf(">")
//    val firstTag = xmlStringTrimmed.substring(1, endOfFirstTag)
//
//    val endOfLastTag = xmlStringTrimmed.lastIndexOf(firstTag) + 1 + firstTag.size
//
//    val cleanedXmlString = xmlStringTrimmed.substring(0, endOfLastTag)
//
//    println("CLEANED XML: \n" + cleanedXmlString)

    val cleanedXmlString = xmlString.trim

    val xml: Elem = scala.xml.XML.loadString(cleanedXmlString)
    xml match {
      case interest @ <interest>{_*}</interest> => {
        val nameComponents = parseComponents(interest)
        println(s"Interest with name: ${nameComponents.mkString("/")}")
        Interest(nameComponents)
      }
      case content @ <contentobj>{_*}</contentobj> => {
        val nameComponents = parseComponents(content)
        val contentData = parseContent(content)
        println(s"Content with name: ${nameComponents.mkString("/")} and data: ${new String(contentData)}")
        Content(nameComponents, contentData)
      }
      case _ => throw new Exception("XML parser cannot parse:\n" + xml)
    }
  }

  def main(args: Array[String]) = {

    println("NFNCommunication - main")
    val socket = UDPClient()
    val ccnIf = new CCNLiteInterface()

    val interestName = "/name/of/interest"
    val interest: Array[Byte] = ccnIf.mkBinaryInterest(interestName)
//    println("client will send:\n" + ccnIf.ccnbToXml(interest))
    socket.sendReceive(interest) onComplete {
      case Success(respInterest) => {
        val xmlDataInterest = ccnIf.ccnbToXml(respInterest)
        println(s"ccnb2XML interest:\n$xmlDataInterest")
        //    println("client received response (to xml):\n" + xmlData)
        parseXml(xmlDataInterest)
      }
      case Failure(t) => println("An error occured when sending an interest: " + t.getMessage)
    }

    val contentName = "/name/of/content"
    val contentData = "testdata".getBytes
    val content = ccnIf.mkBinaryContent(contentName, contentData)
    val respContent = socket.sendReceive(content) onComplete  {
      case Success(respContent) =>
        val xmlDataContent = ccnIf.ccnbToXml(respContent)
        println(s"ccnb2XML content:\n$xmlDataContent")
        parseXml(xmlDataContent)
      case Failure(t) => println("An error occured when sending an interest: " + t.getMessage)
    }

    Thread.sleep(1000)

  }
}

