package nfn

import akka.actor.{PoisonPill, Props, ActorRef, Actor}
import akka.event.Logging
import ccn.packet.CCNName

object ComputeServer {

  case class Compute(name: CCNName)

  case class Thunk(name: CCNName)

  /**
   * Message to finish computation on compute server
   * @param name
   */
  case class ComputationFinished(name: CCNName)

  case class EndComputation(name: CCNName)
}

case class ComputeServer() extends Actor {
  val logger = Logging(context.system, this)

  private def createComputeWorker(name: CCNName, ccnServer: ActorRef): ActorRef =
    context.actorOf(Props(classOf[ComputeWorker], ccnServer), s"ComputeWorker-${name.hashCode}")

  var computeWorkers = Map[CCNName, ActorRef]()

  override def receive: Actor.Receive = {
    /**
     * Check if computation is already running, if not start a new computation, otherwise do nothing
     */
    case computeMsg @ ComputeServer.Thunk(name: CCNName) => {
      if(name.isCompute) {
        val nameWithoutThunk = name.withoutThunk
        if(!computeWorkers.contains(nameWithoutThunk)) {
          logger.debug(s"Started new computation with thunks for $nameWithoutThunk")
          val computeWorker = createComputeWorker(nameWithoutThunk, sender)
          computeWorkers += nameWithoutThunk -> computeWorker
          computeWorker.tell(computeMsg, sender)
        } else {
          logger.debug(s"Computation for $name is already running")
        }
      } else {
        logger.error(s"Interest was not a compute interest, discaring it")
      }
    }

    case computeMsg @ ComputeServer.Compute(name: CCNName) => {
      if(!name.isThunk) {
        computeWorkers.get(name) match {
          case Some(worker) => {
            logger.debug(s"Received Compute for $name, forwarding it to running compute worker")
            worker.tell(computeMsg, sender)
          }
          case None => {
            logger.debug(s"Started new computation without thunks for $name")
            val computeWorker = createComputeWorker(name, sender)
            computeWorkers += name -> computeWorker
            computeWorker.tell(computeMsg, sender)
          }
        }
      }
      else {
        logger.error(s"Compute message must contain the name of the final interest and not a thunk interest: $name")
      }
    }

    case nack @ ComputeServer.EndComputation(name) => {
      computeWorkers.get(name) match {
        case Some(computeWorker) => {
          computeWorker ! ComputeWorker.End
        }
        case None => logger.warning(s"Received nack for computation which does not exist: $name")
      }

    }

//    case ComputeServer.ComputationFinished(name) => {
//      computeWorkers.get(name) match {
//        case Some(computeWorkerToRemove) => {
//          computeWorkerToRemove ! PoisonPill
//          computeWorkers -= name
//        }
//        case None => logger.warning(s"Received ComputationFinished for $name, but computation doesn't exist anymore")
//      }
//    }
  }
}