package nfn

import akka.actor.{PoisonPill, Props, ActorRef, Actor}
import akka.event.Logging
import ccn.packet.CCNName

object ComputeServer {

  /**
   * Message to start computation on compute server, response will be a content object or a timeout
   * @param name
   * @param useThunks
   */
  case class Compute(name: CCNName, useThunks: Boolean)

  /**
   * Message to finish computation on compute server
   * @param name
   */
  case class ComputationFinished(name: CCNName)
}

case class ComputeServer() extends Actor {
  val logger = Logging(context.system, this)

  private def createComputeWorker(name: CCNName, ccnServer: ActorRef): ActorRef =
    context.actorOf(Props(classOf[ComputeWorker], ccnServer), s"ComputeWorker-${name.hashCode}")

  var computeWorkers = Map[CCNName, ActorRef]()

  override def receive: Actor.Receive = {
    case computeMsg @ ComputeServer.Compute(name: CCNName, useThunks) => {
      if(!computeWorkers.contains(name)) {
        val computeWorker = createComputeWorker(name, sender)
        computeWorkers += name -> computeWorker
        val initialRequestor = sender
        computeWorker.tell(computeMsg, initialRequestor)
      }
    }

    case ComputeServer.ComputationFinished(name) => {
      computeWorkers.get(name) map { computeWorkerToRemove =>
        computeWorkerToRemove ! PoisonPill
        computeWorkers -= name
      }
    }
  }
}