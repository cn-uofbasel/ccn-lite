import akka.actor._
import com.typesafe.scalalogging.slf4j.Logging
import lambdacalculus.machine.{ConstValue, ValuePrettyPrinter, Value, CallExecutor}
import lambdacalculus.{ExecutionOrder, LambdaCalculus}
import language.experimental.macros

import LambdaMacros._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Await}
import scala.concurrent.duration._
import scala.util.{Failure, Success, Try}

import ExecutionContext.Implicits.global


case class WordCountService() extends NFNService {

  def countWords(doc: NFNName) = 42

  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues match {
      case Seq(docNameString) => Seq(
        NFNNameValue(NFNName(Seq(docNameString)))
      )
      case _ => throw new Exception(s"Service $toNFNName could not parse single Int value from: '$unparsedValues'")
    }
    val name = NFNName(Seq(unparsedName))
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

  override def toNFNName: NFNName = NFNName(Seq("WordCountService/Int/rInt"))
}


case class AddService() extends  NFNService with Logging {
  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    logger.debug("parse")
    val values = unparsedValues match {
      case Seq(lStr, rStr) => Seq(
          NFNIntValue(lStr.toInt),
          NFNIntValue(rStr.toInt)
      )
      case _ => throw new Exception(s"Service $toNFNName could not parse two int values from: '$unparsedValues'")
    }
    val name = NFNName(Seq(unparsedName))
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

  override def toNFNName: NFNName = NFNName(Seq("/AddService/Int/Int/rInt"))
}

case class SumService() extends  NFNService {
  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues.map( (unparsedValue: String) => NFNIntValue(unparsedValue.toInt))
    val name = NFNName(Seq(unparsedName))
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

  override def toNFNName: NFNName = NFNName(Seq("SumService/Int/Int/rInt"))
}

case class NFNMapReduce(values: NFNNameList, map: NFNService, reduce: NFNService)

case class NFNNameList(names: List[NFNName])

case class LocalNFNCallExecutor() extends CallExecutor {
  override def executeCall(call: String): Value = {
    NFNService.parseAndFindFromName(call).flatMap({ serv => serv.exec}) match {
      case Success(NFNIntValue(n)) => ConstValue(n)
      case Success(res @ _) => throw new Exception(s"LocalNFNCallExecutor: Result of executeCall is not implemented for: $res")
      case Failure(e) => throw new Exception(e)
    }
  }
}

case class ScalaToLocalMachine() extends NFNSender with Logging {
  val lc = LambdaCalculus(ExecutionOrder.CallByValue, maybeExecutor = Some(LocalNFNCallExecutor()))

  override def nfnSend(lambda: String): String = {
    lc.substituteParseCompileExecute(lambda) match {
      case Success(List(v: Value)) => ValuePrettyPrinter(v)
      case Success(List(values @ _*)) => {
        logger.warn("More than one execution result")
        values.mkString("[", " | ", "]")
      }
      case Success(Nil) => throw new Exception("ScalaToLocalMachine: For some reason there was a successful execution without an result")
      case Failure(e) => throw new Exception(e)
    }

  }
}

trait NFNSender {
  def apply(lambda:String): String = nfnSend(lambda)
  def nfnSend(lambda: String): String
}

case class ScalaToNFN() extends NFNSender with Logging {

//  val system = ActorSystem("NFNActorSystem")

//  val worker = system.actorOf(Props(new NFNWorker("NFNWroker")), name = "NFNWorker")
//  val udpConnection = system.actorOf(Props(new UDPConnection(worker, port = 9001)), name = "UDPListener")
//  Thread.sleep(2000)

  //  val simpleSender = system.actorOf(Props(new SimpleUDPSender), name = "SimpleUDPSender")
  //
  //  val interest = "add 1 1/NFN"
  //  udpConnection !  Send(new CCNLiteInterface().mkBinaryInterest(interest))

  logger.debug("Started ScalaToNfn...")

  val socket = UDPClient("NFNSocket", 9000)
  val ccnIf = new CCNLiteInterface()


  new Thread(
    UDPServer("ComputeServer", 9001, "localhost", { data =>
    val maybeContent =
      NFNCommunication.parseXml(ccnIf.ccnbToXml(data)) match {
        case Interest(cmps) => {
          val computeCmps = cmps.slice(1, cmps.size-1)
          Some(
            NFNService.parseAndFindFromName(computeCmps.mkString(" ")) flatMap { _.exec } match {
              case Success(nfnServiceValue) => nfnServiceValue match {
                case NFNIntValue(n) => Content(cmps, n.toString.getBytes)
                case res @ _ => throw new Exception(s"Compute Server response is only implemented for results of type NFNIntValue and not: $res")
              }
              case Failure(e) => throw e
            }
          )
        }
        case c: Content => logger.warn(s"Compute socket received content packet $c, discarding it"); None
      }
//    maybeContent map { content =>
//      ccnIf.mkBinaryContent(content.name.toArray, content.data)
//    }
    maybeContent map { content =>
      val binaryContent = ccnIf.mkBinaryContent(content.name.toArray, content.data)
      socket.send(binaryContent)
    }
    None
  })
  ).start



  override def nfnSend(lambda: String): String = {

    val interest = Interest(Seq(lambda))

    println(ccnIf.mkBinaryInterest(interest.nameComponents.toArray))
    val binaryInterest = ccnIf.mkBinaryInterest(interest.nameComponents.toArray)
    println(ccnIf.ccnbToXml(binaryInterest))
    val f = socket.sendReceive(binaryInterest)

    NFNCommunication.parseXml(ccnIf.ccnbToXml(Await.result(f, 60 seconds))) match {
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
      case Interest(name) => logger.warn(s"NFNSocket received interest with name '$name', discarding it"); throw new Exception(s"NFNSocket received interest with name '$name', discarding it")
      //val serv = NFNServiceLibrary.find(NFNName(name))
    }
  }

}

object ScalaToNFN extends App with Logging {

//  val nfnSend = ScalaToNFN()
  val nfnSend = ScalaToNFN()

  val res = nfnSend(
    "add 7 (call 3 /AddService/Int/Int/rInt 11 24)"
//    lambda{
//    val x = 41
//    x + 1
//    val dollar = 100
//    NFNServiceLibrary.convertDollarToChf(100)
//    def fun(x: Int): Int = x + 1
//    fun(2)
//    val dollar = 100
//    dollar + 1
//  }
)
  println(s"Result: $res")

  Thread.sleep(10000)

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

