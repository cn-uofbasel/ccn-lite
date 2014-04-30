package ccn.packet

sealed trait Packet

/**
 * Marker trait for acknowledgement
 */
trait Ack

sealed trait CCNPacket extends Packet {
  def name: Seq[String]
  def nameComponents:Seq[String] = name ++ Seq("NFN")
}

case class Interest(name: Seq[String]) extends CCNPacket {
  def this(nameStr: String) = this(Seq(nameStr))
  override def toString = s"Interest('${name}')"
}
case class Content(name: Seq[String], data: Array[Byte]) extends CCNPacket {

  def this(nameStr: String, data: Array[Byte]) = this(Seq(nameStr), data)
  override def toString = s"Content('$name' => ${new String(data)})"
}

case class AddToCache() extends Packet with Ack
