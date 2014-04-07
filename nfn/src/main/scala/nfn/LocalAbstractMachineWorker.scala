package nfn

import akka.actor._
import akka.event.Logging
import ccn.packet._
import lambdacalculus.LambdaCalculus
import lambdacalculus.machine.{Value, ListValue, ConstValue}
import nfn.NFNMaster.{CCNAddToCache, ComputeResult}
import scala.util.{Success, Failure, Try}
import ccn.ContentStore

/**
 * Created by basil on 01/04/14.
 */
class LocalAbstractMachineWorker(nfnMaster: ActorRef) extends Actor {
  val logger = Logging(context.system, this)

  val lc = LambdaCalculus(maybeExecutor = Some(LocalNFNCallExecutor(nfnMaster)))

  override def receive: Actor.Receive = {
    case content: Content => {
      val senderCopy = sender
      handleContent(content, senderCopy)
    }
    case interest: Interest =>
      val senderCopy = sender
      handleInterest(interest, senderCopy)
  }

  private def handleContent(content: Content, sender: ActorRef) = {
    logger.info(s"Received content $content, adding to contentstore")
    nfnMaster ! CCNAddToCache(content)
  }


  // Returns the content for the interest if it is in the contentstore
  // otherwise no response
  private def handleInterest(interest: Interest, sender: ActorRef) = {

    def computeResultToContent(computeResult: Value): String = computeResult match {
      case ConstValue(n, _) => n.toString
      case ListValue(values, _) => (values map { computeResultToContent }).mkString(" ")
      case r@_ => throw new Exception(s"Result is only implemented for type ConstValue and not $r")
    }

    def handleComputeRequest(interest: Interest) = {
      def tryComputeResultContent: Try[Content] = {
        // TODO this only works if the expression is in a single name and not split
        if (interest.name.size == 2) {
          val expr = interest.name.init.mkString(" ")
          lc.substituteParseCompileExecute(expr) map {
            case List(result: Value) => {
              val resultString = computeResultToContent(result)
              println(s"ResultString: $resultString")
              Content(interest.name, resultString.getBytes)
            }
            case results@_ => throw new Exception(s"Local abstract machine: Result of exeuction contains more or less than 1 element: $results")
          }
        }
        else throw new Exception(s"Local abstract machine can only parse compute requests with the form <lambda expr><NFN> and not $interest")
      }

      tryComputeResultContent match {
        case Success(content) => {
          logger.info(s"Computed content $content")
          sender ! content
        }
        case Failure(e) => logger.error(e.getMessage)
      }
    }

    logger.info(s"Received interest $interest")
    handleComputeRequest(interest)
  }
}

