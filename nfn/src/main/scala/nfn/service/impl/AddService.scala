package nfn.service.impl

import nfn.service._
import com.typesafe.scalalogging.slf4j.Logging


case class AddService() extends  NFNService with Logging {
  def instantiateCallable(name: NFNName, values: Seq[NFNServiceValue]): CallableNFNService = {
    logger.debug(s"AddService: InstantiateCallable(name: $name, toNFNName: $toNFNName")

    assert(name == toNFNName, s"Service $toNFNName is created with wrong name $name")

    values match {
      case Seq(v1: NFNIntValue, v2: NFNIntValue) =>  {
        CallableNFNService(name,
                           values,
                           { (values: Seq[NFNServiceValue]) =>
                             values match {
                               case Seq(l: NFNIntValue, r: NFNIntValue) => {
                                 NFNIntValue(l.amount + r.amount)
                               }
                               case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
                             }
                           }
        )

      }
      case _ => throw new Exception(s"Service $toNFNName can only be applied to two NFNIntValues and not '$values'")
    }
  }

  override def toNFNName: NFNName = NFNName(Seq("AddService", "Int", "Int", "rInt"))
}
