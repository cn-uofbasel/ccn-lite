package nfn

import akka.actor._
import akka.event.Logging
import ccn.packet._
import nfn.service.ContentStore
import lambdacalculus.LambdaCalculus
import lambdacalculus.machine.ConstValue
import nfn.NFNMaster.ComputeResult
import scala.util.{Success, Failure, Try}

/**
 * Created by basil on 01/04/14.
 */
class LocalAbstractMachineWorker extends Actor {
  val logger = Logging(context.system, this)

  val lc = LambdaCalculus(maybeExecutor = Some(LocalNFNCallExecutor(self)))

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
    ContentStore.add(content)
  }


  // Returns the content for the interest if it is in the contentstore
  // otherwise no response
  private def handleInterest(interest: Interest, sender: ActorRef) = {
    def handleContentRequest(interest: Interest) = {
      ContentStore.find(interest.name) match {
        case Some(content) => {
          logger.debug(s"Found content for interest $interest")
          sender ! content
        }
        case None => if(interest.name.last == "NFN") {
          logger.debug(s"Received compute request")
          handleComputeRequest(interest)
        }
      }
    }

    def handleComputeRequest(interest: Interest) = {
      // TODO this only works if
      val content: Try[Content] =
        if(interest.name.size == 2) {
          val expr = interest.name.init.mkString(" ")
          lc.substituteParseCompileExecute(expr) map {
            case List(result) => result match {
              case ConstValue(n, _) => Content(interest.name, n.toString.getBytes)
              case r @ _ => throw new Exception(s"Result is only implemented for type ConstValue and not $r")
            }
            case results @ _ => throw new Exception(s"Local abstract machine: Result of exeuction contains more or less than 1 element: $results")
          }
        }
        else throw new Exception(s"Local abstract machine can only parse compute requests with the form <lambda expr><NFN> and not $interest")

      content match {
        case Success(content) => sender ! content
        case Failure(e) => logger.error(e.getMessage)
      }
    }

    logger.info(s"Received interest $interest")
    interest.name match {
      case "COMPUTE" :: rest => handleComputeRequest(interest)
      case _ =>  handleContentRequest(interest)
    }

  }
}

