package nfn.service

import bytecode.BytecodeLoader
import java.io.{FileWriter, FileOutputStream, File}
import language.experimental.macros


import scala.reflect.runtime.{universe => ru}
import scala.util.matching.Regex
import scala.util.{Failure, Success, Try}
import ccn.ccnlite.CCNLite
import nfn.service.impl._
import ccn.packet.{Interest, Content}
import akka.actor.ActorRef
import com.typesafe.scalalogging.slf4j.Logging
import scala.concurrent.Future
import nfn.NFNMaster.{CCNAddToCache, CCNSendReceive}
import akka.pattern._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.util.Timeout
import scala.concurrent.duration._
import scala.concurrent._
import scala.collection.mutable

case class ContentStore() extends Logging {
  private val cs: mutable.Map[Seq[String], Content] = mutable.Map()

  def add(content: Content) = {
    cs += (content.name -> content)
  }

  def find(name: Seq[String]):Option[Content] = cs.get(name)

  def remote(name: Seq[String]): Unit = cs -= name
}


object NFNServiceLibrary extends Logging {
  private var services:Map[Seq[String], NFNService] = Map()
  private val ccnIf = CCNLite

  add(AddService())
  add(WordCountService())

  def add(serv: NFNService) =  {

    val name = serv.nfnName.name
    services += name -> serv
  }

  def find(servName: String):Option[NFNService] = {
    servName.split("/") match {
      case Array("", servNameCmps @ _*) =>
        logger.debug(s"Looking for: '$servNameCmps' in '$services'")
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

case class NFNBinaryDataValue(name: Seq[String], data: Array[Byte]) extends NFNServiceValue {
  override def toValueName: NFNName = NFNName(Seq(new String(data)))

  override def toNFNName: NFNName = NFNName(name)
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

case class CallableNFNService(name: NFNName, values: Seq[NFNServiceValue], function: (Seq[NFNServiceValue]) => NFNServiceValue) {
  def exec:NFNServiceValue = function(values)
}

case class NFNName(name: Seq[String]) {
  override def toString = name.mkString("/", "/", "")
}

trait NFNServiceValue {


  def toNFNName: NFNName

  def toValueName: NFNName
}



trait NFNService extends Logging {

  def function: (Seq[NFNServiceValue]) => NFNServiceValue

  def verifyArgs(args: Seq[NFNServiceValue]): Try[Seq[NFNServiceValue]]

  def instantiateCallable(name: NFNName, values: Seq[NFNServiceValue]): Try[CallableNFNService] = {
    logger.debug(s"AddService: InstantiateCallable(name: $name, toNFNName: $nfnName")
    assert(name == nfnName, s"Service $nfnName is created with wrong name $name")
    verifyArgs(values)
    Try(CallableNFNService(name, values, function))
  }
//  def instantiateCallable(name: NFNName, futValues: Seq[Future[NFNServiceValue]], ccnWorker: ActorRef): Future[CallableNFNService]

  def nfnName: NFNName = NFNName(Seq(this.getClass.getCanonicalName.replace(".", "_")))


  def pinned: Boolean = true

}

object NFNService extends Logging {
  implicit val timeout = Timeout(20 seconds)

  def parseAndFindFromName(name: String, ccnWorker: ActorRef): Future[CallableNFNService] = {

    def loadFromCacheOrNetwork(interest: Interest): Future[Content] = (ccnWorker ? CCNSendReceive(interest)).mapTo[Content]
    def findService(fun: String): Future[NFNService] = {
      NFNServiceLibrary.find(fun) match {
        case Some(serv) => {
          future {
            serv
          }
        }
        case None => {

          val interest = Interest(fun.split("/").tail.toSeq)
          val futServiceContent: Future[Content] = loadFromCacheOrNetwork(interest)
          futServiceContent flatMap { content =>
            // writes the content data to a temporary file, loadas the class from this file and deletes the file again
            // TODO: this is ugly, either clean up or find a better solution (meaning to load a class directly from a byte array
            println(s"Found service in cache, loading class from content object")
            val serviceLibraryDir = "./service-library"
            def createTempFile: File = {
              val f = new File(s"$serviceLibraryDir/${System.nanoTime}")
              if (!f.exists) {
                f
              } else {
                Thread.sleep(1)
                createTempFile
              }
            }

            var out: FileOutputStream = null
            val file: File = createTempFile
            try {
              out = new FileOutputStream(file)
              out.write(content.data)
            } finally {
              if (out != null) out.close
              if (file.exists) file.delete
            }
            val servName = content.name.head.replace("_", ".")

            val loadedService = BytecodeLoader.loadClass[NFNService](serviceLibraryDir, servName)

            logger.debug(s"Dynamically loaded class $servName from content")

            import myutil.Implicit.tryToFuture
            loadedService
          }
        }
      }
    }

    def findArgs(args: List[String]): Future[List[NFNServiceValue]] = {
      Future.sequence(
        args map { (arg: String) =>
          arg.forall(_.isDigit) match {
            case true => {
              future{
                NFNIntValue(arg.toInt)
              }
            }
            case false => {
              val interest = Interest(arg.split("/").tail)

              loadFromCacheOrNetwork(interest) map  { content =>
                NFNBinaryDataValue(content.name, content.data)
              }
            }
          }
        }
      )
    }



    logger.debug(s"Trying to find service for: $name")
    val pattern = new Regex("""^call (\d)+ (.*)$""")

    name match {
      case pattern(countString, funArgs) => {
        val l = funArgs.split(" ").toList
        val (fun, args) = (l.head, l.tail)

        val count = countString.toInt - 1
        assert(count == args.size, s"matched name $name is not a valid service call, because arg count (${count + 1}) is not equal to number of args (${args.size}) (currently nfn counts the function name itself as an arg)")
        assert(count > 0, s"matched name $name is not a valid service call, because count cannot be 0 or smaller (currently nfn counts the function name itself as an arg)")

        // find service
        val futServ = findService(fun)

        // create or find values
        val futArgs: Future[List[NFNServiceValue]] = findArgs(args)

        import myutil.Implicit.tryToFuture

        val futCallableServ: Future[CallableNFNService] =
          for {
            args <- futArgs
            serv <- futServ
            callable <- serv.instantiateCallable(serv.nfnName, args)

          } yield callable

        futCallableServ onSuccess {
          case callableServ => logger.info(s"Instantiated callable serv: '$name' -> $callableServ")
        }
        futCallableServ
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

