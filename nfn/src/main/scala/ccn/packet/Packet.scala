package ccn.packet

sealed trait Packet {
  def name: Seq[String]
  def nameComponents:Seq[String] = name ++ Seq("NFN")
}

case class Interest(name: Seq[String]) extends Packet {

  def this(nameStr: String) = this(Seq(nameStr))

  override def toString = s"Interest('${name}')"
}

case class Content(name: Seq[String], data: Array[Byte]) extends Packet {

  def this(nameStr: String, data: Array[Byte]) = this(Seq(nameStr), data)
  override def toString = s"Content('$name' => ${new String(data)})"
}