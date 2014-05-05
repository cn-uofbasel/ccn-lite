package ccn

import ccn.packet.CCNName
import akka.actor.ActorRef

case class PendingInterestTable[FaceType]() {
  var pit: Map[CCNName, Set[FaceType]] = Map()

  def get(name: CCNName):Option[Set[FaceType]] = pit.get(name)
  def add(name: CCNName, face: FaceType) = {
    get(name) match {
      case Some(faces) =>
        pit += (name -> faces.+(face))
      case None =>
        pit += (name -> Set(face))
    }
  }
}
