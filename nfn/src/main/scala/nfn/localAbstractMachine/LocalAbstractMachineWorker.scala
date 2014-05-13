package nfn.localAbstractMachine

import akka.actor._
import akka.event.Logging
import ccn.packet._
import lambdacalculus._
import lambdacalculus.machine.{Value, ListValue, ConstValue}
import scala.util.{Success, Failure, Try}
import nfn.NFNApi

/**
 * Encapsulates a single abstract machine, reduces lambdacalculus expressions and sends requests for content back to the [[ccnServer]].
 * @param ccnServer
 */
class LocalAbstractMachineWorker(ccnServer: ActorRef) extends Actor {

  val logger = Logging(context.system, this)
  val lc = LambdaCalculus(execOrder = ExecutionOrder.CallByValue,
                          debug = true,
                          storeIntermediateSteps = true,
                          maybeExecutor = Some(LocalNFNCallExecutor(ccnServer)),
                          parser = new NFNLambdaParser)

  override def receive: Actor.Receive = {
    case content: Content => {
      logger.debug(s"Received content $content")
      handleContent(content, sender)
    }
    case interest: Interest =>
      logger.debug(s"Received interest $interest")
      handleInterest(interest, sender)
  }

  private def handleContent(content: Content, senderCopy: ActorRef) = {
    logger.warning(s"Discarding content $content")
  }


  // Returns the content for the interest if it is in the contentstore
  // otherwise no response
  private def handleInterest(interest: Interest, senderCopy: ActorRef) = {

    def computeResultToContent(computeResult: Value): String = computeResult match {
      case ConstValue(n, _) => n.toString
      case ListValue(values, _) => (values map { computeResultToContent }).mkString(" ")
      case r@_ => throw new Exception(s"Result is only implemented for type ConstValue and not $r")
    }

    def tryComputeContentForExpr(expr: String): Try[Content] = {
      lc.substituteParseCompileExecute(expr) map {
        case List(result: Value) => {
          val resultString = computeResultToContent(result)
          Content(interest.name, resultString.getBytes)
        }
        case results@_ => throw new Exception(s"Local abstract machine: Result of execution contains more or less than one element: $results")
      }
    }

    def handleNFNRequest(interest: Interest) = {
      def tryComputeResultContent: Try[Content] = {
        // TODO this only works if the expression is in a single name and not split
        interest.name.cmps match {
          case Seq(lambdaExpr, "NFN") => tryComputeContentForExpr(lambdaExpr)
          case Seq(postCmp, preExpr, "NFN") => tryComputeContentForExpr(s"$preExpr $postCmp")
          case _ => throw new Exception(s"Local abstract machine can only parse compute requests " +
                                        s"with the form <lambda expr><NFN> or <postcmp><lambdaexpr><NFN> and not $interest")
        }
      }

      tryComputeResultContent match {
        case Success(content) => {
          logger.info(s"Computed content $content")
          senderCopy ! content
        }
        case Failure(e) => logger.error(e, s"Could not compute the result for the interest $interest")
      }
    }

    handleNFNRequest(interest)
  }
}

