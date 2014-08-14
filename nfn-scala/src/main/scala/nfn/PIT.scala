package nfn

import ccn.packet.{CCNPacket, Interest, CCNName}
import akka.actor.{Actor, ActorRef}
import scala.collection.mutable
import akka.actor.Actor.Receive
import scala.concurrent.duration._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.event.Logging

trait Face {
  def send(ccnPacket: CCNPacket)
}

case class ActorRefFace(actorRef: ActorRef) extends Face {
  def send(ccnPacket: CCNPacket) {
    actorRef ! ccnPacket
  }
}

case class PendingInterest(name: CCNName, faces: List[Face], timeout: Long) extends Ordered[PendingInterest] {

  val startTime = System.nanoTime

  def timer = startTime + (timeout * 1000000)

  override def compare(that: PendingInterest): Int = (this.timer - that.timer).toInt
}

object PIT {
  case class Add(name: CCNName, face: Face, timeout: FiniteDuration)

  case class Remove(name: CCNName)

  case class Get(name: CCNName)

  case class Update()

}

case class PIT() extends Actor {

  val logger = Logging(context.system, this)

  private case class Timeout(name: CCNName, face: Face)

  private val pit = mutable.Map[CCNName, Set[Face]]()

  override def receive: Receive = {
    case PIT.Add(name, face, timeout) => {
      pit += name -> (pit.get(name).getOrElse(Set()) + face)
      context.system.scheduler.scheduleOnce(timeout) {
        self ! Timeout(name, face)
      }
    }
    case PIT.Get(name) => {
      val maybePendingFaces =
        pit.get(name) map { pendingFaces =>
          pit -= name
          pendingFaces
        }
      sender ! maybePendingFaces
    }

    case Timeout(name, face) => {
      pit.get(name) map { pendingFaces =>
        logger.warning(s"Timing out interest: $name to face $face")
        pit += name -> (pendingFaces - face)
      }
    }

    case PIT.Remove(name) => {
      pit -= name
    }
  }
}
