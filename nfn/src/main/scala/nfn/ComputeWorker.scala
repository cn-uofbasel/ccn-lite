package nfn

import akka.actor.{Actor, ActorRef}
import akka.event.Logging
import ccn.ccnlite.CCNLite
import ccn.packet.{CCNName, Interest, Content}
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import nfn.service.{NFNValue, NFNService, CallableNFNService}
import scala.util.{Failure, Success}
import myutil.IOHelper
import ComputeWorker._

/**
 * Created by basil on 23/04/14.
 */
object ComputeWorker {
  case class Exit()
}

/**
 *
 */
case class ComputeWorker(ccnServer: ActorRef) extends Actor {

  val name = self.path.name
  val logger = Logging(context.system, this)
  val ccnIf = CCNLite

  private var result : Option[String] = None

  def receivedContent(content: Content) = {
    // Received content from request (sendrcv)
    logger.error(s"ComputeWorker received content, discarding it because it does not know what to do with it")
  }

  // Received compute request
  // Make sure it actually is a compute request and forward to the handle method
  def receivedComputeRequest(name: CCNName, requestor: ActorRef) = {
    logger.debug(s"received compute request: $name")
    name.cmps match {
      case Seq("COMPUTE", reqNfnCmps @ _ *) => {
        assert(reqNfnCmps.last == "NFN")
        val (computeCmps, isThunkReq) = CCNName(reqNfnCmps.init:_*).withoutThunkAndIsThunk

        handleComputeRequest(computeCmps, name, isThunkReq, requestor)
      }
      case _ => logger.error(s"Dropping interest $name because it is not a compute request")
    }
  }


  /*
   * Parses the compute request and instantiates a callable service.
   * On success, sends a thunk back if required, executes the services and sends the result back.
   */
  def handleComputeRequest(computeName: CCNName, originalName: CCNName, isThunkRequest: Boolean, requestor: ActorRef) = {
    logger.debug(s"Handling compute request for name: $computeName")
    val futCallableServ: Future[CallableNFNService] = NFNService.parseAndFindFromName(computeName.cmps.mkString(" "), ccnServer)

    futCallableServ onComplete {
      case Success(callableServ) => {
        if(isThunkRequest) {
          // TODO: No default value for network
          requestor ! Content(originalName, callableServ.executionTimeEstimate.fold("")(_.toString).getBytes)
        }

        val result: NFNValue = callableServ.exec
        val content = Content(originalName.withoutThunkAndIsThunk._1, result.toStringRepresentation.getBytes)
        logger.info(s"Finished computation, result: $content")
        requestor ! content
      }
      case Failure(e) => {

        logger.error(IOHelper.exceptionToString(e))
      }
    }
  }

  override def receive: Actor.Receive = {
    case ComputeServer.Compute(name, useThunks) => {
      receivedComputeRequest(name, sender)
    }
    case Exit() => context.stop(self)
  }
}
