package nfn.service

import java.io.{File, FileOutputStream}

import akka.actor._
import akka.pattern._
import akka.util.Timeout
import bytecode.BytecodeLoader
import ccn.packet._
import com.sun.xml.internal.ws.policy.privateutil.PolicyUtils.ConfigFile
import com.typesafe.config.ConfigFactory
import com.typesafe.scalalogging.slf4j.Logging
import config.AkkaConfig
import lambdacalculus.parser.ast._
import nfn.{StaticConfig, NodeConfig, NFNApi}

import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent._
import scala.util.Try

object NFNService extends Logging {

  implicit val timeout = Timeout(StaticConfig.defaultTimeoutDuration)

  /**
   * Creates a [[NFNService]] from a content object containing the binary code of the service.
   * The data is written to a temporary file, which is passed to the [[BytecodeLoader]] which then instantiates the actual class.
   * @param content
   * @return
   */
  def serviceFromContent(content: Content): Try[NFNService] = {
//    println(s"serviceFromContent: content size=${content.data.size}")
//    val serviceLibraryDir = "./service-library"
//    val serviceLibararyFile = new File(serviceLibraryDir)
//
//    if(serviceLibararyFile.exists) {
//      serviceLibararyFile.mkdir
//    }
//
//    def createTempFile: File = {
//      val f = new File(s"$serviceLibraryDir/${System.nanoTime}")
//      if (!f.exists) {
//        f
//      } else {
//        Thread.sleep(1)
//        createTempFile
//      }
//    }
//
//    var out: FileOutputStream = null
//    val file: File = createTempFile
//    try {
//      out = new FileOutputStream(file)
//      out.write(content.data)
//      out.flush()
    val servName = content.name.cmps.head.replace("_", ".")
    val fileName = new String(content.data)
    val filePath = new File(fileName).getCanonicalPath
    val loadedService: Try[NFNService] = BytecodeLoader.loadClass[NFNService](filePath, servName)
    logger.debug(s"Dynamically loaded class $servName from content")
    loadedService
//      loadedService
//    } finally {
//      if (out != null) out.close
////      if (file.exists) file.delete
//    }
  }

  def parseAndFindFromName(name: String, ccnServer: ActorRef): Future[CallableNFNService] = {

    def loadFromCacheOrNetwork(interest: Interest): Future[Content] = {
      (ccnServer ? NFNApi.CCNSendReceive(interest, useThunks = false)).mapTo[Content]
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

              import nfn.LambdaNFNImplicits._
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

trait NFNService {

  def executionTimeEstimate: Option[Int] = None

  def function: (Seq[NFNValue], ActorRef) => NFNValue

  def instantiateCallable(name: CCNName, values: Seq[NFNValue], ccnServer: ActorRef, executionTimeEstimate: Option[Int]): Try[CallableNFNService] = {
    assert(name == ccnName, s"Service $ccnName is created with wrong name $name")
    Try(CallableNFNService(name, values, ccnServer, function, executionTimeEstimate))
  }
//  def instantiateCallable(name: NFNName, futValues: Seq[Future[NFNServiceValue]], ccnWorker: ActorRef): Future[CallableNFNService]

  def ccnName: CCNName = CCNName(this.getClass.getCanonicalName.replace(".", "_"))

  def pinned: Boolean = false

  override def toString = ccnName.toString

}

abstract class NFNDynamicService() extends NFNService {
  override def ccnName = CCNName("nfn_DynamicService")
}