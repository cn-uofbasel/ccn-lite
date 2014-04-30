package ccn.packet

sealed trait Packet

/**
 * Marker trait for acknowledgement
 */
trait Ack

object CCNName {
//  def apply(cmps: String *): CCNName = CCNName(cmps.toList)
  def withAddedNFNComponent(ccnName: CCNName) = CCNName(ccnName.cmps ++ Seq("NFN") :_*)
  def withAddedNFNComponent(cmps: Seq[String]) = CCNName(cmps ++ Seq("NFN") :_*)
}


case class CCNName(cmps: String *) {
  def to = toString.replaceAll("/", "_").replaceAll("[^a-zA-Z0-9]", "-")
  override def toString = cmps.mkString("/", "/", "")
}

sealed trait CCNPacket extends Packet {
  def name: CCNName
}

object NFNInterest {
  def apply(cmps: String *): Interest = Interest(CCNName(cmps :_*))
}

object Interest {
  def apply(cmps: String *): Interest = Interest(CCNName(cmps :_*))
}
case class Interest(name: CCNName) extends CCNPacket {
  def this(cmps: String *) = this(CCNName(cmps:_*))
}

object Content {
  def apply(data: Array[Byte], cmps: String *): Content =
    Content(CCNName(cmps :_*), data)
}

case class Content(name: CCNName, data: Array[Byte]) extends CCNPacket {

//  def this(data: Array[Byte], cmps: String *) = this(CCNName(cmps.toList), data)
  def possiblyShortenedDataString: String = {
    val dataString = new String(data)
    if(dataString.length > 20) dataString.take(20) + "..." else dataString
  }
  override def toString = s"Content('$name' => $possiblyShortenedDataString)"
}

case class AddToCache() extends Packet with Ack
