package nfn.service

import bytecode.BytecodeLoader
import java.io.{FileOutputStream, File}
import language.experimental.macros


import scala.reflect.runtime.{universe => ru}
import scala.util.matching.Regex
import scala.util.Try
import ccn.ccnlite.CCNLite
import nfn.service.impl._
import ccn.packet.Content
import akka.actor.ActorRef
import network.Send
import com.typesafe.scalalogging.slf4j.Logging
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.global


object NFNServiceLibrary extends Logging {

  private var services:Map[String, NFNService] = Map()
  private val ccnIf = CCNLite

  add(AddService())

  def add(serv: NFNService) =  {

    val name = serv.toNFNName.toString
    services += name -> serv
  }

  def find(servName: String):Option[NFNService] = {
    logger.debug(s"Looking for: '$servName' in '$services'")
    services.get(servName)
//    TODO: if not found, ask NFN
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
   * @param nfnSocket
   */
  def nfnPublish(nfnSocket: ActorRef) = {

    def pinnedData = "pinnedfunction".getBytes
    def maybeByteCodeData(serv: NFNService):Option[Array[Byte]] = {
      val bc = BytecodeLoader.fromClass(serv)
      bc match {
        case Some(bc) => logger.info(s"nfnPublish: Found bytecode for service $serv")
        case None => logger.warn(s"nfnPublush: No bytecode found for unpinned service $serv")
      }
      bc
    }

    for((name, serv) <- services) {
      val content = Content(
        Seq(name),
        if(serv.pinned) pinnedData
        else maybeByteCodeData(serv).getOrElse(pinnedData)
      )
      val binaryInterest = ccnIf.mkAddToCacheInterest(content)
      nfnSocket ! Send(binaryInterest)
    }
  }

  // TODO: Best would be to abstract a general send - receive mechanism
//  def nfnRequest(nfnSocket: ActorRef, servName: NFNName): Future[Option[NFNService]] = {
//  }

  def convertChfToDollar(chf: Int): Int = ???
  def toPdf(webpage: String): String = ???
  def derp(foo: Int) = ???
}

case class NFNIntValue(amount: Int) extends NFNServiceValue {
  def apply = amount

  override def toNFNName: NFNName = NFNName(Seq("Int"))

  override def toValueName: NFNName = NFNName(Seq(amount.toString))
}

case class NFNNameValue(name: NFNName) extends NFNServiceValue{
  override def toValueName: NFNName = name

  override def toNFNName: NFNName = name
}

case class NFNServiceException(msg: String) extends Exception(msg)

case class DollarToChf() extends NFNService {

  override def toNFNName:NFNName = NFNName(Seq("DollarToChf/Int/rInt"))

  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues match {
      case Seq(dollarValueString) => Seq(NFNIntValue(dollarValueString.toInt))
      case _ => throw new Exception(s"Service $toNFNName could not parse single Int value from: '$unparsedValues'")
    }
    val name = NFNName(Seq(unparsedName))
    assert(name == this.toNFNName)

    val function = { (values: Seq[NFNServiceValue]) =>
      values match {
        case Seq(dollar: NFNIntValue) => {
          Try(NFNIntValue(dollar.amount/2))
        }
        case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
      }

    }
    CallableNFNService(name, values, function)
  }
}

case class CallableNFNService(name: NFNName, values: Seq[NFNServiceValue], function: (Seq[NFNServiceValue]) => Try[NFNServiceValue]) {
  def exec:Try[NFNServiceValue] = function(values)
}

case class NFNName(name: Seq[String]) {
  override def toString = name.mkString("/")
}

trait NFNServiceValue {

  def toNFNName: NFNName

  def toValueName: NFNName
}



trait NFNService {

  def parse(name: String, values: Seq[String]):CallableNFNService

  def toNFNName: NFNName

  def pinned: Boolean = true
}

object NFNService extends Logging {
  def parseAndFindFromName(name: String): Try[CallableNFNService] = {

    logger.debug(s"Trying to find service for: $name")
    val pattern = new Regex("""^call (\d)+ (.*)$""")

    name match {
      case pattern(countString, funArgs) => {
        val l = funArgs.split(" ").toList
        val (fun, args) = (l.head, l.tail)

        val count = countString.toInt - 1
        assert(count == args.size, s"matched name $name is not a valid service call, because arg count (${count + 1}) is not equal to number of args (${args.size}) (currently nfn counts the function name itself as an arg)")
        assert(count > 0, s"matched name $name is not a valid service call, because count cannot be 0 or smaller (currently nfn counts the function name itself as an arg)")

        val serv = NFNServiceLibrary.find(fun).getOrElse(throw new Exception(s"parseAndFindFromName: Service $fun not found in service library"))

        val callableServ = serv.parse(fun, args)
        logger.debug(s"Found service for request: $callableServ")

        Try(callableServ)
      }
      case unvalidServ @ _ => throw new Exception(s"matched name $name (parsed to: $unvalidServ) is not a valid service call, because arg count is not equal nto number of args (currently nfn counts the function name itself as an arg)")
    }
  }
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
    val service = BytecodeLoader.loadClassFromJar[NFNService](jarfile, "NFNServiceTestImpl")

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

