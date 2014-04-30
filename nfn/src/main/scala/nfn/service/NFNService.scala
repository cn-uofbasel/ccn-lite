package nfn.service

import scala.util.matching.Regex
import scala.util.Try
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
import nfn.NFNMaster._

object NFNService extends Logging {

  implicit val timeout = Timeout(20 seconds)

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

  def parseAndFindFromName(name: String, ccnWorker: ActorRef): Future[CallableNFNService] = {

    def loadFromCacheOrNetwork(interest: Interest): Future[Content] = {
      (ccnWorker ? CCNSendReceive(interest)).mapTo[Content]
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
          val interest = Interest(fun.split("/").tail.toSeq :_*)
          val futServiceContent: Future[Content] = loadFromCacheOrNetwork(interest)
          import myutil.Implicit.tryToFuture
          futServiceContent flatMap { serviceFromContent }
        }
      }
    }

    def findArgs(args: List[String]): Future[List[NFNValue]] = {
      logger.debug(s"Looking for args $args")
      Future.sequence(
        args map { (arg: String) =>
          arg.forall(_.isDigit) match {
            case true => {
              logger.debug(s"Arg '$arg' is a number")
              future{
                NFNIntValue(arg.toInt)
              }
            }
            case false => {
              val interest = Interest(arg.split("/").tail.toList :_*)
              logger.debug(s"Arg '$arg' is a name, asking nfn master to find content for $interest")
              loadFromCacheOrNetwork(interest) map  { content =>
                logger.debug(s"Found arg $arg")
                NFNBinaryDataValue(content.name, content.data)
              }
            }
          }
        }
      )
    }



    logger.debug(s"Trying to find service for: $name")
    val pattern = new Regex("""^call ([\d]+) (.*)$""")

    name match {
      case pattern(countString, funArgs) => {
        val l = funArgs.split(" ").toList
        val (fun, args) = (l.head, l.tail)

        val count = countString.toInt - 1
        assert(count == args.size, s"matched name $name is not a valid service call, because arg count (${count}) is not equal to number of args (${args.size}) (currently nfn counts the function name itself as an arg)")
        assert(count >= 0, s"matched name $name is not a valid service call, because count cannot be 0 or smaller (currently nfn counts the function name itself as an arg)")

        // find service
        val futServ = findService(fun)

        // create or find values
        val futArgs: Future[List[NFNValue]] = findArgs(args)

        import myutil.Implicit.tryToFuture

        val futCallableServ: Future[CallableNFNService] =
          for {
            args <- futArgs
            serv <- futServ
            callable <- serv.instantiateCallable(serv.ccnName, args)

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

trait NFNService extends Logging {

  def function: (Seq[NFNValue]) => NFNValue

  def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]]

  def argumentException(args: Seq[NFNValue]):NFNServiceArgumentException


  def instantiateCallable(name: CCNName, values: Seq[NFNValue]): Try[CallableNFNService] = {
    logger.debug(s"NFNService: InstantiateCallable(name: $name, values: $values")
    assert(name == ccnName, s"Service $ccnName is created with wrong name $name")
    verifyArgs(values)
    Try(CallableNFNService(name, values, function))
  }
//  def instantiateCallable(name: NFNName, futValues: Seq[Future[NFNServiceValue]], ccnWorker: ActorRef): Future[CallableNFNService]

  def ccnName: CCNName = CCNName(this.getClass.getCanonicalName.replace(".", "_"))


  def pinned: Boolean = true

  override def toString = ccnName.toString

}