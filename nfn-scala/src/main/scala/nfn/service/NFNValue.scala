package nfn.service

import ccn.packet.CCNName


trait NFNValue {
  def toCCNName: CCNName

  def toStringRepresentation: String
}

case class NFNBinaryDataValue(name: CCNName, data: Array[Byte]) extends NFNValue {

  override def toCCNName: CCNName = name

  override def toStringRepresentation: String = new String(data)
}

case class NFNServiceValue(serv: NFNService) extends NFNValue {
  override def toStringRepresentation: String = serv.ccnName.toString

  override def toCCNName: CCNName = serv.ccnName
}



case class NFNListValue(values: List[NFNValue]) extends NFNValue {

  override def toStringRepresentation: String = values.map({ _.toCCNName.toString }).mkString(" ")

  override def toCCNName: CCNName = CCNName(values.map({ _.toCCNName.toString }).mkString(" "))
}

case class NFNNameValue(name: CCNName) extends NFNValue{
  override def toStringRepresentation: String = name.toString

  override def toCCNName: CCNName = name
}

case class NFNIntValue(amount: Int) extends NFNValue {
  def apply = amount

  override def toCCNName: CCNName = CCNName("Int")

  override def toStringRepresentation: String = amount.toString
}

case class NFNStringValue(str: String) extends NFNValue {
  def apply = str

  override def toCCNName: CCNName = CCNName("String")

  override def toStringRepresentation: String = str
}

case class NFNEmptyValue() extends NFNValue {

  override def toCCNName: CCNName = CCNName("EmptyValue")

  override def toStringRepresentation: String = ""
}
