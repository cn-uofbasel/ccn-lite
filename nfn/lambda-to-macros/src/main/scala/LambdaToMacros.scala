import com.typesafe.scalalogging.slf4j.Logging
import language.experimental.macros

import LambdaMacros._
import scala.collection.immutable
import scala.concurrent.Await
import scala.concurrent.duration._
import scala.util.Try


case class WordCountService() extends NFNService {

  def countWords(doc: NFNName) = 42

  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues match {
      case Seq(docNameString) => Seq(
        NFNNameValue(NFNName.parse(docNameString).getOrElse(throw new NFNServiceException(s"Could not create NFNName for docname: $docNameString")))
      )
      case _ => throw new Exception(s"Service $toNFNName could not parse single Int value from: '$unparsedValues'")
    }
    val name = NFNName.parse(unparsedName).getOrElse(throw new Exception(s"Service $toNFNName could not parse function name '$unparsedName'"))
    assert(name == this.toNFNName)

    val function = { (values: Seq[NFNServiceValue]) =>
      values match {
        case Seq(docName: NFNNameValue) => { Try(
          NFNIntValue(countWords(docName.name))
        )}
        case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
      }

    }
    CallableNFNService(name, values, function)
  }

  override def toNFNName: NFNName = NFNName("WordCountService/Int/rInt")
}


case class AddService() extends  NFNService {
  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues match {
      case Seq(lStr, rStr) => Seq(
          NFNIntValue(lStr.toInt),
          NFNIntValue(rStr.toInt)
      )
      case _ => throw new Exception(s"Service $toNFNName could not parse two int values from: '$unparsedValues'")
    }
    val name = NFNName.parse(unparsedName).getOrElse(throw new Exception(s"Service $toNFNName could not parse function name '$unparsedName'"))
    assert(name == this.toNFNName)

    val function = { (values: Seq[NFNServiceValue]) =>
      values match {
        case Seq(l: NFNIntValue, r: NFNIntValue) => { Try(
          NFNIntValue(l.amount + r.amount)
        )}
        case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
      }

    }
    CallableNFNService(name, values, function)
  }

  override def toNFNName: NFNName = NFNName("SumService/Int/Int/rInt")
}

case class SumService() extends  NFNService {
  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues.map( (unparsedValue: String) => NFNIntValue(unparsedValue.toInt))
    val name = NFNName.parse(unparsedName).getOrElse(throw new Exception(s"Service $toNFNName could not parse function name '$unparsedName'"))
    assert(name == this.toNFNName)

    val function = { (values: Seq[NFNServiceValue]) =>
      Try(NFNIntValue(
        values.map({
          case NFNIntValue(i) => i
          case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
        }).sum
      ))
    }
    CallableNFNService(name, values, function)
  }

  override def toNFNName: NFNName = NFNName("SumService/Int/Int/rInt")
}

case class NFNMapReduce(values: NFNNameList, map: NFNService, reduce: NFNService)

case class NFNNameList(names: List[NFNName])

object ScalaToNFN extends App with Logging {

  val socket = UDPClient()
  val ccnIf = new CCNLiteInterface()

  def nfnSend(lambda: String): String = {

    val interest = Interest(lambda)

    val binaryInterest = ccnIf.mkBinaryInterest(interest.nameComponents)
    println(ccnIf.ccnbToXml(binaryInterest))
    val f = socket.sendReceive(binaryInterest)

    NFNCommunication.parseXml(ccnIf.ccnbToXml(Await.result(f, 5 seconds))) match {
      case Content(name, data) => {
        val dataString = new String(data)
        val resultPrefix = "RST|"

        val resultContentString = dataString.startsWith(resultPrefix) match {
          case true => dataString.substring(resultPrefix.size)
          case false => throw new Exception(s"NFN could not compute result for: $interest")
        }
        logger.info(s"NFN: '$lambda' => '$resultContentString'")
        resultContentString
      }

      case Interest(name) =>
        NFNName.parse(name) match {
          case Some(nfnName) => val serv = NFNServiceLibrary.find(nfnName)
          case None => throw new Exception(s"Not a valid nfn name: $interest")
        }
        // TODO
        throw new Exception(s"Received a Interest from NFN. not implemented")
    }
  }

  println("Running ScalaToNFN...")

  val res = nfnSend(lambda{
//    val x = 41
//    x + 1
//    val dollar = 100
//    NFNServiceLibrary.convertDollarToChf(100)
//    def fun(x: Int): Int = x + 1
//    fun(2)
    val dollar = 100
    dollar + 1
  })
  println(s"Result: $res")


}

object LambdaToMacro extends App {

  NFNServiceLibrary.add(WordCountService())
  NFNServiceLibrary.add(SumService())
  NFNServiceLibrary.add(AddService())

  println(lambda(
    {
      val dollar = 100
      dollar + 1
    }))

//  println(lambda(
//  {
//    val succ = (x: Int) => 1 + x
//    succ(2)
//  }
//  ))
//
//  println(lambda( {
//      val dollar = 100
//      def suc(x: Int, y: Int): Int = x + 1
//      NFNServiceLibrary.convertChfToDollar(NFNServiceLibrary.convertDollarToChf(dollar))
//  }))
//
//  println(lambda({
//    NFNName("name/of/doc1")
//  }))
//  println(lambda({
//    val x = 1
//    if(x > 1) 2
//    else 0
//  }))
//
//  println(lambda({
//     val list = immutable.List(NFNName("name/of/doc1"), NFNName("name/of/doc2"), NFNName("name/of/doc3"))
//  }))
//  println(lambda({
//  //    val list = NFNName("name/of/doc1") :: NFNName("name/of/doc2") :: NFNName("name/of/doc3") :: Nil
//  }))
}

