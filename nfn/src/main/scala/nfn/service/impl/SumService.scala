package nfn.service.impl

import nfn.service._
import scala.util.Try
import nfn.service.NFNName
import nfn.service.NFNIntValue
import nfn.service.CallableNFNService

/**
 * Created by basil on 24/03/14.
 */
case class SumService() extends  NFNService {
  override def parse(unparsedName: String, unparsedValues: Seq[String]): CallableNFNService = {
    val values = unparsedValues.map( (unparsedValue: String) => NFNIntValue(unparsedValue.toInt))
    val name = NFNName(Seq(unparsedName))
    assert(name == this.toNFNName)

    val function = { (values: Seq[NFNServiceValue]) =>
      Try(NFNIntValue(
        values.map({
          case NFNIntValue(i) => i
          case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
        }).sum
      ))
    }
    CallableNFNService(name, values, function)
  }

  override def toNFNName: NFNName = NFNName(Seq("SumService/Int/Int/rInt"))
}
