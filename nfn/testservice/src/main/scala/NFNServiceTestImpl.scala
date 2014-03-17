import scala.util.Try

/**
 * Created by basil on 19/02/14.
 */

case class UnitValue() extends NFNServiceValue {
  override def toValueName: NFNName = NFNName("Unit")

  override def toNFNName: NFNName = NFNName("Unit")
}

/*
 * Example of a NFNService implementation
 * To add a service to NFN, implement the NFNService interface and
 * add the jar to the local NFNInterface.
 */
class NFNServiceTestImpl extends NFNService {
//  override def exec = println("Test NFNService class loaded successfully!")
  override def exec(values: NFNServiceValue*): Try[NFNServiceValue] = {
    println("Executed test service!")
    Try(UnitValue())
  }
  override def toNFNName: NFNName = NFNName("")
}


case class ChfToDollar() extends NFNService {
  override def exec(values: NFNServiceValue*): Try[NFNIntValue] = {
    values match {
      case Seq(dollarValue:NFNIntValue) => Try(
        NFNIntValue((dollarValue.apply * 1.1).toInt)
      )
      case _ => throw new Exception(s"Service $toNFNName only accepts single Int as a parameter")
    }
  }

  override def toNFNName: NFNName = NFNName("ChfToDollar/Int/Int")
}

