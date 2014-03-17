package ccn

import network.Packet

trait ContentName {
  def name: String
}
case class ContentNameImpl(name: String) extends ContentName

trait Content

case class Data[A](data: A) extends Content
case class NoContent() extends Content

case class Interest(name: ContentName) extends Packet
case class DataPacket(name: ContentName, data: Content) extends Packet

