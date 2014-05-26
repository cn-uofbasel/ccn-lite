package nfn.service

import scala.util.matching.Regex
import scala.util.{Failure, Success, Try}
import scala.concurrent.duration._
import scala.concurrent._
import scala.concurrent.ExecutionContext.Implicits.global

import java.io.{FileOutputStream, File}
import com.typesafe.scalalogging.slf4j.Logging

import akka.pattern._
import akka.util.Timeout
import akka.actor._

import ccn.packet._
import bytecode.BytecodeLoader
import nfn.NFNServer._
import nfn.{LambdaNFNImplicits, LambdaNFNPrinter, NFNApi}
import config.AkkaConfig
import lambdacalculus.parser.ast._

object NFNService extends Logging {

  implicit val timeout = AkkaConfig.timeout

  /**
   * Creates a [[NFNService]] from a content object containing the binary code of the service.
   * The data is written to a temporary file, which is passed to the [[BytecodeLoader]] which then instantiates the actual class.
   * @param content
   * @return
   */
  def serviceFromContent(content: Content): Try[NFNService] = {
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
    val servName = content.name.cmps.head.replace("_", ".")

    val loadedService = BytecodeLoader.loadClass[NFNService](serviceLibraryDir, servName)

    logger.debug(s"Dynamically loaded class $servName from content")
    loadedService
  }

  def parseAndFindFromName(name: String, ccnServer: ActorRef): Future[CallableNFNService] = {

    def loadFromCacheOrNetwork(interest: Interest): Future[Content] = {
      (ccnServer ? NFNApi.CCNSendReceive(interest)).mapTo[Content]
    }

    def findService(fun: String): Future[NFNService] = {
      logger.debug(s"Looking for service $fun")
      NFNServiceLibrary.find(fun) match {
        case Some(serv) => {
          future {
            serv
          }
        }
        case None => {
          CCNName.fromString(fun) match {
            case Some(interestName) =>
              val interest = Interest(interestName)
              val futServiceContent: Future[Content] = loadFromCacheOrNetwork(interest)
              import myutil.Implicit.tryToFuture
              futServiceContent flatMap { serviceFromContent }
            case None => Future.failed(new Exception(s"Could not create name for service $fun"))
          }
        }
      }
    }

    def findArgs(args: List[Expr]): Future[List[NFNValue]] = {
      logger.debug(s"Looking for args ${args.mkString("[ ", ", ", " ]")}")
      Future.sequence(
        args map { (arg: Expr) =>
          arg match {
            case Constant(i) => {
              logger.debug(s"Arg '$arg' is a number")
              Future(
                NFNIntValue(i)
              )
            }
            case otherExpr @ _ => {
              import LambdaDSL._
              import LambdaNFNImplicits._
              val maybeInterest = otherExpr match {
                case Variable(varName, _) => {
                  CCNName.fromString(varName) map {
                    Interest(_)
                  }
                }
                case _ => Some(NFNInterest(otherExpr))
              }

              maybeInterest match {
                case Some(interest) => {
                  logger.debug(s"Arg '$arg' is a name, asking the ccnServer to find content for $interest")
                  val foundContent: Future[NFNBinaryDataValue] = loadFromCacheOrNetwork(interest) map  { content =>
                    logger.debug(s"Found $content for arg $arg")
                    NFNBinaryDataValue(content.name, content.data)
                  }

                  foundContent.onFailure {
                    case error => logger.error(s"Could not find content for arg $arg", error)
                  }

                  foundContent
                }
                case None => {
                  val errorMsg = s"Could not created interest for arg $arg)"
                  logger.error(errorMsg)
                  Future.failed(new Exception(errorMsg))
                }
              }
            }
          }
        }
      )
    }

    val lc = lambdacalculus.LambdaCalculus()

    val res: Try[Future[CallableNFNService]] =
      lc.parse(name) map { (parsedExpr: Expr) =>
        val r: Future[CallableNFNService] =
          parsedExpr match {
            case Call(funName, argExprs) => {

              // find service
              val futServ: Future[NFNService] = findService(funName)

              findArgs(argExprs).zip(Future(argExprs))
              // create or find values for args
              val futArgs: Future[List[NFNValue]] = findArgs(argExprs)

              import myutil.Implicit.tryToFuture
              val futCallableServ: Future[CallableNFNService] =
                for {
                  args <- futArgs
                  serv <- futServ
                  callable <- serv.instantiateCallable(serv.ccnName, args, ccnServer, serv.executionTimeEstimate)
                } yield callable

              futCallableServ onSuccess {
                case callableServ => logger.info(s"Instantiated callable serv: '$name' -> $callableServ")
              }
              futCallableServ
            }
            case _ => throw new Exception("call is the only valid expression for a COMPUTE request")
          }
        r
      }
    import myutil.Implicit.tryFutureToFuture
    res
  }
}

trait NFNService extends Logging {

  def executionTimeEstimate: Option[Int] = None

  def function: (Seq[NFNValue], ActorRef) => NFNValue

  def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]]

  def argumentException(args: Seq[NFNValue]):NFNServiceArgumentException

  def instantiateCallable(name: CCNName, values: Seq[NFNValue], ccnServer: ActorRef, executionTimeEstimate: Option[Int]): Try[CallableNFNService] = {
    logger.debug(s"NFNService: InstantiateCallable(name: $name, values: $values")
    assert(name == ccnName, s"Service $ccnName is created with wrong name $name")
    verifyArgs(values)
    Try(CallableNFNService(name, values, ccnServer, function, executionTimeEstimate))
  }
//  def instantiateCallable(name: NFNName, futValues: Seq[Future[NFNServiceValue]], ccnWorker: ActorRef): Future[CallableNFNService]

  def ccnName: CCNName = CCNName(this.getClass.getCanonicalName.replace(".", "_"))


  def pinned: Boolean = false

  override def toString = ccnName.toString

}