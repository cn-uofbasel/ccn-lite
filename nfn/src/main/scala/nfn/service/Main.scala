package nfn.service

import language.experimental.macros


import scala.reflect.runtime.{universe => ru}
import scala.util.matching.Regex
import scala.util._
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.duration._
import scala.concurrent._
import scala.collection.mutable

import java.io.{FileOutputStream, File}

import com.typesafe.scalalogging.slf4j.Logging

import akka.actor.ActorRef
import akka.pattern._
import akka.util.Timeout

import ccn.ccnlite.CCNLite
import ccn.packet._
import nfn.service.impl._
import nfn.NFNMaster._
import bytecode.BytecodeLoader




object NFNServiceLibrary extends Logging {
  private var services:Map[Seq[String], NFNService] = Map()
  private val ccnIf = CCNLite

  add(AddService())
  add(WordCountService())
  add(MapService())
  add(ReduceService())
  add(SumService())

  def add(serv: NFNService) =  {

    val name = serv.nfnName.name
    services += name -> serv
  }

  def find(servName: String):Option[NFNService] = {
    servName.split("/") match {
      case Array("", servNameCmps @ _*) =>
        val found = services.get(servNameCmps)
        logger.debug(s"Found $found")
        found
      case _ => {
        logger.error(s"Could not split name $servName with '/'")
        None
      }
    }
  }


  def find(servName: NFNName):Option[NFNService] = find(servName.toString)

  def convertDollarToChf(dollar: Int): Int = ???
//    val serv = DollarToChf()
//    val servRes = serv.exec(IntValue(dollar)).get
//    servRes match {
//      case intValue: IntValue => intValue.amount
//      case _ => throw new Exception(s"${serv.toName} did not return a IntValue but a $servRes")
//    }
//  }

  /**
   * Advertises all locally available services to nfn by sending a 'addToCache' Interest,
   * containing a content object with the name of the service (and optionally also the bytecode * of the service).
   * @param ccnWorker
   */
  def nfnPublish(ccnWorker: ActorRef) = {

    def pinnedData = "pinnedfunction".getBytes

    def byteCodeData(serv: NFNService):Array[Byte] = {
      BytecodeLoader.fromClass(serv) match {
        case Some(bc) => bc
        case None =>
          logger.error(s"nfnPublush: No bytecode found for unpinned service $serv")
          pinnedData
      }
    }

    for((_, serv) <- services) {

      val serviceContent =
        if(serv.pinned) pinnedData
        else byteCodeData(serv)

      val content = Content(
        serv.nfnName.name,
        serviceContent
      )

      logger.debug(s"nfnPublish: Adding ${content.name} to cache")
      ccnWorker ! CCNAddToCache(content)
    }
  }

  // TODO: Best would be to abstract a general send - receive mechanism
//  def nfnRequest(nfnSocket: ActorRef, servName: NFNName): Future[Option[NFNService]] = {
//  }

  def convertChfToDollar(chf: Int): Int = ???
  def toPdf(webpage: String): String = ???
  def derp(foo: Int) = ???
}







case class NFNServiceExecutionException(msg: String) extends Exception(msg)
case class NFNServiceArgumentException(msg: String) extends Exception(msg)

//case class DollarToChf() extends NFNService {
//
//  override def toNFNName:NFNName = NFNName(Seq("DollarToChf/Int/rInt"))
//
//  override def parse(unparsedName: String, unparsedValues: Seq[String], ccnWorker: ActorRef): Future[CallableNFNService] = {
//    val values = unparsedValues match {
//      case Seq(dollarValueString) => Seq(NFNIntValue(dollarValueString.toInt))
//      case _ => throw new Exception(s"Service $toNFNName could not parse single Int value from: '$unparsedValues'")
//    }
//    val name = NFNName(Seq(unparsedName))
//    assert(name == this.toNFNName)
//
//    val function = { (values: Seq[NFNServiceValue]) =>
//      values match {
//        case Seq(dollar: NFNIntValue) => {
//          Future(NFNIntValue(dollar.amount/2))
//        }
//        case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
//      }
//
//    }
//    Future(CallableNFNService(name, values, function))
//  }
//}

case class CallableNFNService(name: NFNName, values: Seq[NFNValue], function: (Seq[NFNValue]) => NFNValue) {
  def exec:NFNValue = function(values)
}

case class NFNName(name: Seq[String]) {
  override def toString = name.mkString("/", "/", "")
}












object Main {
//  ServiceLibrary.add(DollarToChf())
//
//  val nfn = PrintNFNInterface()
//
//  val lambdaExpression: String =
//    lambda(
//    {
//      val dollar = 100
//      def suc(x: Int, y: Int): Int = x + 1 + y
//      ServiceLibrary.convertChfToDollar(ServiceLibrary.convertDollarToChf(dollar))
//    })
//
//  nfn.send(lambdaExpression)
//
//  val service = "DollarToChf/Int/rInt 100"
//  nfn.exec(service)
  //        {
  //          val dollar = 100
  //          val chf = 50
  //          val test = 0
  //          if(2 > test) Library.convertChfToDollar(chf) else Library.convertDollarToChf(dollar)
  //        }


  def bytecodeLoading = {
    val jarfile = "/Users/basil/Dropbox/uni/master_thesis/code/testservice/target/scala-2.10/testservice_2.10-0.1-SNAPSHOT.jar"
    val service = BytecodeLoader.loadClass[NFNService](jarfile, "NFNServiceTestImpl")
  }

  def findLibraryFunctionWithReflection = {
    def getTypeTag[T: ru.TypeTag](obj: T) = ru.typeTag[T]

    val tpe = getTypeTag(NFNServiceLibrary).tpe

    val methods: List[String] =
      tpe.declarations.filter(decl =>
        decl.isMethod && decl.name.toString != "<init>"
      ).map( methodDecl => {
        def parseTypeString(tpe: String): String = {
          val splitted = tpe.substring(1, tpe.size).split("\\)")
          val params:List[String] = splitted(0).split(", ").map(param => param.substring(param.indexOf(": ")+2, param.size)).toList
          val ret = splitted(1).trim
          s"${params.mkString("/")}/r$ret"
        }
        val name = methodDecl.name.toString
        val tpes = parseTypeString(methodDecl.typeSignature.toString)
        //        typeSignature.map( (tpe: ru.Type) => {
        ////          tpe.toString
        ////        }
        ////        )
        s"/$name/$tpes"
      }
        ).toList

    println(methods.mkString("\n"))
  }
}

