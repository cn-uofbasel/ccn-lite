package ccn

import akka.actor._
import network._
import network.Send
import scala.Some


class FIB() {

  var faces: List[ActorRef] = List()

//  private var fib:Map[ContentName, ActorRef] = Map()

  def longestPrefixMatch(interest: Interest): List[ActorRef] = {
//    if(fib.contains(interest.name)) Some(fib(interest.name))
//    else None
    faces
  }

//  def update(updatedFib: Map[ContentName, ActorRef]) = fib ++= updatedFib

  def addFace(face: ActorRef) = faces = face :: faces

  override def toString: String = faces.toString
}
object FIB {
  def apply(): FIB = new FIB()
}

class CS() {
  private var cs:Map[ContentName, Content] = Map()

  def getOpt(name: ContentName): Option[Content] = {
    if(cs.contains(name)) Some(cs(name))
    else None
  }

  def cache(name: ContentName, content: Content): Unit = {
    if(!cs.contains(name)) cs += (name -> content)
  }

  def cache(dataPacket: DataPacket): Unit = {
    cache(dataPacket.name, dataPacket.data)
  }

  override def toString: String = cs.toString
}

object CS {
  def apply(): CS = new CS()
}

class PIT() {
  private var pit: Map[Interest, List[ActorRef]] = Map()

  def addOrUpdate(interest: Interest, inc: ActorRef): Unit = {
    // TODO: start timer per interest
    if(pit.contains(interest)) {
      pit = pit.updated(interest, inc :: pit(interest))
    } else {
      pit += (interest -> List(inc))
    }
  }

  def requestingFaces(interest: Interest):List[ActorRef] = {
    val rf = pit.getOrElse(interest, List())
    println("removing interest vom pit")
    pit -= interest
    rf
  }

  override def toString: String = pit.toString
}
object PIT {
  def apply(): PIT = new PIT()
}

class CCNClientFace(override val name: String) extends Face {

  var maybeCon: Option[ActorRef] = None

  def con: ActorRef = maybeCon match {
    case Some(con) => con
    case None => throw new Exception("CCNClientFace not connected to a face")
  }

  def receive(packet: Packet, in: ActorRef): Unit = {
    println(s"CLIENT<$name> received: Packet")
  }

  def receive: Actor.Receive = {
    case Connection(con) => maybeCon = Some(con)
    case Send(packet) => con ! Receive(packet)
    case Receive(packet) => receive(packet)
  }
}


class CCNRouterFace(override val name: String, router: CCNRouter) extends Face {

  def receive: Actor.Receive = {
    case Connection(con) => router.addFace(con)
    case Send(packet) => throw new Exception("A router cannot be used to send something directly")
    case Receive(packet) => router.receive(packet, sender)
  }
}

class CCNRouter(name: String) {

  val fib: FIB = FIB()
  val cs: CS = CS()
  val pit = PIT()

  def receive(packet: Packet, in: ActorRef) = {
    println(s"ROUTER received $packet from $in")
    packet match {
      case i: Interest => receiveInterest(i, in)
      case d: DataPacket => receiveDataPacket(d)
    }
  }

  private def receiveInterest(interest: Interest, inc: ActorRef) = {
    cs.getOpt(interest.name) match {
      case Some(content) => {
        println(s"$name: found content for ${interest.name} in CS: ${content}")
        send(DataPacket(interest.name, content), inc)
      }
      case None => {
        println(s"$name: content for ${interest.name} not found in CS")
        pit.addOrUpdate(interest, inc)
        println(s"Updated PIT: $pit")
        forward(interest, inc)
      }
    }
  }

  private def receiveDataPacket(dataPacket: DataPacket) = {
    println(s"$name: received datapacket: $dataPacket")
    cs.cache(dataPacket)
    val outFaces = pit.requestingFaces(Interest(dataPacket.name))
    println(s"$name: returning datapacket to faces: ${outFaces.mkString}")
    outFaces foreach { out =>
      send(dataPacket, out)
    }
  }

  private def forward(interest: Interest, inc: ActorRef) = {
    println(s"$name: forwoarding according to FIB: ${fib.longestPrefixMatch(interest)}")
    fib.longestPrefixMatch(interest) foreach { out =>
      println("out-sending" + inc + "!=" + out)
      if(inc != out) {
        send(interest, out)
      }
    }
  }

  def send(packet: Packet, out: ActorRef) = {
    println(s"$name: sending packet $packet to actor ${out.toString}")
    out ! Receive(packet)
  }

  def addFace(face: ActorRef): Unit = {
    fib.addFace(face)
  }
}

