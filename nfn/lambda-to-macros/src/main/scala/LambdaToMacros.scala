import com.typesafe.scalalogging.slf4j.Logging
import language.experimental.macros

import LambdaMacros._
import scala.collection.immutable
import scala.concurrent.Await
import scala.concurrent.duration._
import scala.util.Try


case class WordCountService() extends NFNService {

  def countWords(doc: NFNName) = 42

  override def exec(values: NFNServiceValue*): Try[NFNServiceValue] = {
    val wc = values match {
      case (NFNNameValue(docName)) :: Nil => countWords(docName)
      case _ => throw new NFNServiceException(s"WordCountService takes only one parameter of type NFNNameValue, which is the name of a document to count the words, and not: $values")
    }

    Try(NFNIntValue(wc))
  }

  override def toNFNName: NFNName = NFNName("WordCountService/Int/rInt")
}


case class AddService() extends  NFNService {
  override def exec(values: NFNServiceValue*): Try[NFNServiceValue] = {
    Try(NFNIntValue(
      values match {
        case List(NFNIntValue(l), NFNIntValue(r)) => r + l
        case _ => throw new NFNServiceException(s"AddService allows only two NFNIntValues as arguments and not $values")
      }
    ))
  }

  override def toNFNName: NFNName = NFNName("SumService/Int/Int/rInt")
}

case class SumService() extends  NFNService {
  override def exec(values: NFNServiceValue*): Try[NFNServiceValue] = {
    Try(NFNIntValue(
      values.map({
          case NFNIntValue(n) => n
          case _ => throw new NFNServiceException("SumService allows only NFNIntValues as arguments")
      }).sum
    ))
  }

  override def toNFNName: NFNName = NFNName("SumService/Int/Int/rInt")
}

case class NFNMapReduce(values: NFNNameList, map: NFNService, reduce: NFNService)

case class NFNNameList(names: List[NFNName])

object ScalaToNFN extends App with Logging {

  val socket = UDPClient()
  val ccnIf = CCNLiteInterface

  val l = lambda{
    val x = 41
    x + 1
  }+"/NFN"
  println(s"lambda<$l>")


  val interest = Interest(
    NFNName.parse(l).getOrElse(
      throw new Exception(s"Lambda Expression '$l' is not a valid NFNName"))
    .name
  )

  val binaryInterest = ccnIf.mkBinaryInterest(interest)
  val f = socket.sendReceive(binaryInterest)
  NFNCommunication.parseXml(ccnIf.ccnbToXml(Await.result(f, 10 seconds))) match {
    case Content(name, data) =>
      val dataString = new String(data)
      val resultPrefix = "RST|"

      val resultContentString = dataString.startsWith(resultPrefix) match {
        case true => dataString.substring(resultPrefix.size)
        case false => throw new Exception(s"NFN could not compute result for: $interest")
      }
      logger.info(s"NFN: '$l' => '$resultContentString'")

    case Interest(name) =>
      NFNName.parse(name) match {
        case Some(nfnName) => val serv = NFNServiceLibrary.find(nfnName)
        case None => throw new Exception(s"Not a valid nfn name: $interest")
      }
      // TODO
      throw new Exception(s"Received a Interest from NFN. not implemented")
  }
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

