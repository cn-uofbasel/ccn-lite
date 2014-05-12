package ccn

import scala.concurrent.ExecutionContext.Implicits.global
import scala.collection.mutable

import akka.actor.Actor
import akka.actor.Actor.Receive
import akka.event.Logging

import ccn.packet._

object CS {

  /**
   * Returns Option[Content] depending on if it was in the content store
   * @param name
   */
  case class Get(name: CCNName)

  /**
   * Returns nothing
   * @param content
   */
  case class Add(content: Content)

  /**
   * Retruns nothing
   * @param name
   */
  case class Remove(name: CCNName)
}

case class CS() extends Actor {
  val logger = Logging(context.system, this)
  private val cs: mutable.Map[CCNName, Content] = mutable.Map()

  override def receive: Receive = {
    case CS.Get(name) => sender ! cs.get(name)
    case CS.Add(content) => cs += (content.name -> content)
    case CS.Remove(name) => cs -= name
  }
}
