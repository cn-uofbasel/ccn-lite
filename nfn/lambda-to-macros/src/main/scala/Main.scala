import bytecode.BytecodeLoader
import language.experimental.macros

import LambdaMacros._

import scala.reflect.runtime.{universe => ru}
import scala.util.{Failure, Success, Try}


import scala.concurrent._
import scala.concurrent.duration._
import ExecutionContext.Implicits.global

object NFNServiceLibrary {

  private var services:Map[String, NFNService] = Map()

  def add(serv: NFNService) =  services += serv.toNFNName.toString -> serv

  def find(servName: String) = services(servName)

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
  override def exec(values: NFNServiceValue*): Try[NFNIntValue] = {
    values match {
      case Seq(dollarValue:NFNIntValue) => Try(
//        IntValue(dollarValue.amount - (dollarValue.amount.toFloat * 0.1f).toInt)
        NFNIntValue(42)
      )
      case _ => throw new Exception(s"Service $toNFNName can only be executed with a single IntValue parameter and not with: $values")
    }
  }

  override def toNFNName:NFNName = NFNName("DollarToChf/Int/rInt")
}

case class NFNName(name: String)

trait NFNServiceValue {

  def toNFNName: NFNName

  def toValueName: NFNName
}


trait NFNService {
  def exec(values: NFNServiceValue*): Try[NFNServiceValue]

  def toNFNName: NFNName
}


/*
 * The NFNInterface represents the interface between the NFN network layer and the application layer.
 * The main functionality is sending an interest for a lambda expression, offering a local executable service to NFN
 * and a function which is called by the network to execute a certain service and return the result as a content object
 */
trait NFNInterface {

  def send(lambdaExpr: String):Unit
  protected def send(serviceCall: String, result: String): Unit


  def exec(serviceCall: String) = {
    val sc = serviceCall.split(" ").toList
    val (fun: String, args: List[String]) = sc.head -> sc.tail.toList

    val splittedFun = fun.split("/").toList

    println(s"splittetfun: $splittedFun")

    val argNames = splittedFun.slice(1, splittedFun.size - 1)
    val retName = splittedFun(splittedFun.size - 1)

    println(s"argnames: $argNames, retname: $retName")


    val argValues:List[NFNServiceValue] = argNames.zip(args).map { case (tpe, value) => tpe match {
        case "Int" => NFNIntValue(0)
        case _ => throw new Exception(s"Could not convert argument with type name $tpe to value $value")
      }
    }

    val serv = NFNServiceLibrary.find(fun)

    val res = serv.exec(argValues:_*)

    val finalResult:Try[NFNServiceValue] =
      res.map {retValue =>
        if("r" + retValue.toNFNName != retName)
          throw new Exception(s"Result value ${"r" + retValue.toNFNName} of service call to $serviceCall is not compatible with actual return value name ${retName}")
        else
          retValue
      }

    finalResult match {
      case Success(retValue: NFNServiceValue) => send(serviceCall, retValue.toValueName.toString)
      case Failure(e) => System.err.println(s"Error when executing service call: ${e.toString}")
    }
  }
}

case class SocketNFNInterface() extends NFNInterface {

  val sock: UDPClient = UDPClient()
  val ccnIf = new CCNLiteInterface()

  override protected def send(serviceCall: String, result: String): Unit = ???

  override def send(lambdaExpr: String): Unit = {

    val interest = ccnIf.mkBinaryInterest(lambdaExpr)

    val futureSend = sock.send(interest)

    val p = promise[Unit]
    p completeWith futureSend

    p.future onComplete {
      case Success(_) => println(s"SocketNFNInterface sent lambdaExpr: $lambdaExpr")
      case Failure(_) => println(s"FAILURE: SocketNFNInterface sent lambdaExpr: $lambdaExpr")
    }
  }
}

case class PrintNFNInterface() extends NFNInterface {
  override def send(lambdaExpr: String) = println(s"NFNSend: $lambdaExpr")
  override protected def send(serviceCall: String, result: String): Unit =  println(s"NFNSend: result<$serviceCall, $result>")
}



object Main {
  def main(args: Array[String]) = {
    val nfnName = "/content/name"
    val nfnSock = SocketNFNInterface()
    println(s"sending $nfnName")
    nfnSock.send(nfnName)
  }
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

