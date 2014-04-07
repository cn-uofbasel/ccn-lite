package nfn.service


trait NFNValue {
  def toNFNName: NFNName

  def toValueName: NFNName
}

case class NFNBinaryDataValue(name: Seq[String], data: Array[Byte]) extends NFNValue {
  override def toValueName: NFNName = NFNName(Seq(new String(data)))

  override def toNFNName: NFNName = NFNName(name)
}

case class NFNServiceValue(serv: NFNService) extends NFNValue {
  override def toValueName: NFNName = serv.nfnName

  override def toNFNName: NFNName = serv.nfnName
}

case class NFNListValue(values: List[NFNValue]) extends NFNValue {

  override def toValueName: NFNName = NFNName(Seq(values.map({ _.toNFNName.toString }).mkString(" ")))

  override def toNFNName: NFNName = NFNName(Seq(values.map({ _.toNFNName.toString }).mkString(" ")))
}

case class NFNNameValue(name: NFNName) extends NFNValue{
  override def toValueName: NFNName = name

  override def toNFNName: NFNName = name
}

case class NFNIntValue(amount: Int) extends NFNValue {
  def apply = amount

  override def toNFNName: NFNName = NFNName(Seq("Int"))

  override def toValueName: NFNName = NFNName(Seq(amount.toString))
}


