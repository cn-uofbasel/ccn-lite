import bytecode.BytecodeLoader
import language.experimental.macros

import LambdaMacros._

import scala.reflect.runtime.{universe => ru}
import scala.util.matching.Regex
import scala.util.matching.Regex.Match
import scala.util.{Failure, Success, Try}


import scala.concurrent._
import scala.concurrent.duration._
import ExecutionContext.Implicits.global

object NFNServiceLibrary {

  private var services:Map[String, NFNService] = Map()
  add(SumService())

  def add(serv: NFNService) =  services += serv.toNFNName.name -> serv

  def find(servName: String):Option[NFNService] = {
    println(s"Looking for: '$servName' in '$services'")
    services.get(servName)
  }


  def find(servName: NFNName):Option[NFNService] = find(servName.name)

  def convertDollarToChf(dollar: Int): Int = ???
//    val serv = DollarToChf()
//    val servRes = serv.exec(IntValue(dollar)).get
//    servRes match {
//      case intValue: IntValue => intValue.amount
//      case _ => throw new Exception(s"${serv.toName} did not return a IntValue but a $servRes")
//    }
//  }
  def convertChfToDollar(chf: Int): Int = ???
  def toPdf(webpage: String): String = ???
  def derp(foo: Int) = ???
}

case class NFNIntValue(amount: Int) extends NFNServiceValue {
  def apply = amount

  override def toNFNName: NFNName = NFNName("Int")

  override def toValueName: NFNName = NFNName(amount.toString)
}

case class NFNNameValue(name: NFNName) extends NFNServiceValue{
  override def toValueName: NFNName = name

  override def toNFNName: NFNName = name
}

case class NFNServiceException(msg: String) extends Exception(msg)

case class DollarToChf() extends NFNService {

  override def toNFNName:NFNName = NFNName("DollarToChf/Int/rInt")

  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues match {
      case Seq(dollarValueString) => Seq(NFNIntValue(dollarValueString.toInt))
      case _ => throw new Exception(s"Service $toNFNName could not parse single Int value from: '$unparsedValues'")
    }
    val name = NFNName.parse(unparsedName).getOrElse(throw new Exception(s"Service $toNFNName could not parse function name '$unparsedName'"))
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

case class NFNName(name: String)

object NFNName {

  def parse(name: String): Option[NFNName] = {
    parse(name.split("/"))
  }

  def parse(components: Seq[String]): Option[NFNName] = {
    if(components.forall { cmp => !cmp.contains("/") && cmp != "" })
      Some(NFNName(components.mkString("/")))
    else
      None
  }
}

trait NFNServiceValue {

  def toNFNName: NFNName

  def toValueName: NFNName
}



trait NFNService {

  def parse(name: String, values: Seq[String]):CallableNFNService

  def toNFNName: NFNName
}

object NFNService {
  def parseAndFindFromName(name: String): Try[CallableNFNService] = {

    println(s"Name: $name")
//    val pattern = new Regex("""^call (\d)+ ([\S]+) ([\S]+) ([\S]+)$""")
    val pattern = new Regex("""^call (\d)+ (.*)$""")

    name match {
      case pattern(countString, funArgs) => {
        val l = funArgs.split(" ").toList
        val (fun, args) = (l.head, l.tail)

        println(s"fun: $fun")
        println(s"args: $args")

        val count = countString.toInt - 1
        assert(count == args.size, s"matched name $name is not a valid service call, because arg count (${count + 1}) is not equal to number of args (${args.size}) (currently nfn counts the function name itself as an arg)")
        assert(count > 0, s"matched name $name is not a valid service call, because count cannot be 0 or smaller (currently nfn counts the function name itself as an arg)")
//
        val serv = NFNServiceLibrary.find(fun).getOrElse(throw new Exception(s"parseAndFindFromName: Service $fun not found in service library"))

        Try(serv.parse(fun, args))
      }
//      case Nil => throw new Exception(s"No name could be parsed from: $name")
      case unvalidServ @ _ => throw new Exception(s"matched name $name (parsed to: $unvalidServ) is not a valid service call, because arg count is not equal nto number of args (currently nfn counts the function name itself as an arg)")
    }
  }
}



/*
 * The NFNInterface represents the interface between the NFN network layer and the application layer.
 * The main functionality is sending an interest for a lambda expression, offering a local executable service to NFN
 * and a function which is called by the network to execute a certain service and return the result as a content object
 */
//trait NFNInterface {
//
//  def send(interest: Interest):Future[Content]
//
//  def receive(interest: Interest):
//
//  def exec(serviceCall: String) = {
//    NFNService.parseAndFindFromName(serviceCall).map({serv => serv.exec}) match {
//      case Success(retValue: Try[NFNServiceValue]) => send(Content(serviceCall, retValue)
//      case Failure(e) => System.err.println(s"Error when executing service call: ${e.toString}")
//    }
//  }
//}

//case class SocketNFNInterface() extends NFNInterface {
//
//  val sock: UDPClient = UDPClient()
//  val ccnIf = CCNLiteInterface
//
//  override protected def send(serviceCall: String, result: String): Unit = ???
//
//  override def send(lambdaExpr: String): Unit = {
//
//    val interest = ccnIf.mkBinaryInterest(Interest(lambdaExpr))
//
//    val futureSend = sock.send(interest)
//
//    val p = promise[Unit]
//    p completeWith futureSend
//
//    p.future onComplete {
//      case Success(_) => println(s"SocketNFNInterface sent lambdaExpr: $lambdaExpr")
//      case Failure(_) => println(s"FAILURE: SocketNFNInterface sent lambdaExpr: $lambdaExpr")
//    }
//  }
//}
//
//case class PrintNFNInterface() extends NFNInterface {
//  override def send(lambdaExpr: String) = println(s"NFNSend: $lambdaExpr")
//  override protected def send(serviceCall: String, result: String): Unit =  println(s"NFNSend: result<$serviceCall, $result>")
//}



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

//  def scalaCodeToMacro = {
//    val l: String = lambda(
  //    val lc = LambdaCalculus(ExecutionOrder.CallByValue, debug = true, storeIntermediateSteps = false)
  //    lc.substituteParseCompileExecute(l) match {
  //      case Success(resultStack: List[Value]) => {
  //        println(s"resultstack: $resultStack")
  //        val strResult: List[String] = resultStack.map(ValuePrettyPrinter(_, Some(lc.compiler)))
  //        println(strResult.mkString(",")) //resultStack.map(ValuePrettyPrinter(_, Some(lambdaCalculus.compiler))).mkString(","))
  //      }
  //      case Failure(error) => {
  //        System.err.println(error)
  //      }
  //    }
//  }
}

