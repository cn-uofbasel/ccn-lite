import akka.actor._
import ccn.packet.Interest
import ccnliteinterface.CCNLiteInterface
import com.typesafe.scalalogging.slf4j.Logging
import lambdacalculus.machine._
import lambdacalculus.{ExecutionOrder, LambdaCalculus}
import language.experimental.macros

import LambdaMacros._
import nfn.{LocalNFNCallExecutor, ComputeWorker}
import nfn.service.impl.{WordCountService, AddService}
import scala.concurrent.{ExecutionContext, Await}
import scala.concurrent.duration._
import scala.Some
import scala.util.{Failure, Success, Try}

import ExecutionContext.Implicits.global
import nfn.service._
import language.experimental.macros



trait NFNSender {
  def apply(lambda:String): String = nfnSend(lambda)
  def nfnSend(lambda: String): String
}

case class ScalaToLocalMachine() extends NFNSender with Logging {
//  val lc = LambdaCalculus(ExecutionOrder.CallByValue, maybeExecutor = Some(LocalNFNCallExecutor(ccnWorker)))

  override def nfnSend(lambda: String): String = {
//    lc.substituteParseCompileExecute(lambda) match {
//      case Success(List(v: Value)) => ValuePrettyPrinter(v)
//      case Success(List(values @ _*)) => {
//        logger.warn("More than one execution result")
//        values.mkString("[", " | ", "]")
//      }
//      case Success(Nil) => throw new Exception("ScalaToLocalMachine: For some reason there was a successful execution without an result")
//      case Failure(e) => throw new Exception(e)
//    }
    ???

  }
}

//case class ScalaToNFN(nfnSocket: ActorRef, computeSocket: ActorRef) extends NFNSender with Logging {
//
//  val ccnIf = new CCNLiteInterface()
//
////  override def nfnSend(lambda: String): String = {
////
////    val interest = Interest(Seq(lambda))
////
////    println(ccnIf.mkBinaryInterest(interest.nameComponents.toArray))
////    val binaryInterest = ccnIf.mkBinaryInterest(interest.nameComponents.toArray)
////    println(ccnIf.ccnbToXml(binaryInterest))
////    val f = socket.sendReceive(binaryInterest)
////
////    NFNCommunication.parseXml(ccnIf.ccnbToXml(Await.result(f, 60 seconds))) match {
////      case Content(name, data) => {
////        val dataString = new String(data)
////        val resultPrefix = "RST|"
////
////        val resultContentString = dataString.startsWith(resultPrefix) match {
////          case true => dataString.substring(resultPrefix.size)
////          case false => throw new Exception(s"NFN could not compute result for: $interest")
////        }
////        logger.info(s"NFN: '$lambda' => '$resultContentString'")
////        resultContentString
////      }
////      case Interest(name) => logger.warn(s"NFNSocket received interest with name '$name', discarding it"); throw new Exception(s"NFNSocket received interest with name '$name', discarding it")
////      //val serv = NFNServiceLibrary.find(NFNName(name))
////    }
////  }
//  override def nfnSend(lambda: String): String = {
//    val interest = Interest(Seq(lambda))
//
//    ccnIf.mkBinaryInterest(interest)
//  }
//}

//object ScalaToNFN extends App with Logging {
//
////  val nfnSend = ScalaToNFN()
//  val nfnSend = ScalaToNFN()
//
//  val res = nfnSend(
//    "add 7 (call 3 /AddService/Int/Int/rInt 11 24)"
////    lambda{
////    val x = 41
////    x + 1
////    val dollar = 100
////    NFNServiceLibrary.convertDollarToChf(100)
////    def fun(x: Int): Int = x + 1
////    fun(2)
////    val dollar = 100
////    dollar + 1
////  }
//)
//  println(s"Result: $res")
//
//  Thread.sleep(10000)
//
//}




//object LambdaToMacro extends App {
//
//  NFNServiceLibrary.add(WordCountService())
//  NFNServiceLibrary.add(SumService())
//  NFNServiceLibrary.add(AddService())
//
//  println(lambda(
//    {
//      val dollar = 100
//      dollar + 1
//    }))
//
////  println(lambda(
////  {
////    val succ = (x: Int) => 1 + x
////    succ(2)
////  }
////  ))
////
////  println(lambda( {
////      val dollar = 100
////      def suc(x: Int, y: Int): Int = x + 1
////      NFNServiceLibrary.convertChfToDollar(NFNServiceLibrary.convertDollarToChf(dollar))
////  }))
////
////  println(lambda({
////    NFNName("name/of/doc1")
////  }))
////  println(lambda({
////    val x = 1
////    if(x > 1) 2
////    else 0
////  }))
////
////  println(lambda({
////     val list = immutable.List(NFNName("name/of/doc1"), NFNName("name/of/doc2"), NFNName("name/of/doc3"))
////  }))
////  println(lambda({
////  //    val list = NFNName("name/of/doc1") :: NFNName("name/of/doc2") :: NFNName("name/of/doc3") :: Nil
////  }))
//}

