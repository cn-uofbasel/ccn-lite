package nfn.service.impl

import nfn.service._
import com.typesafe.scalalogging.slf4j.Logging
import scala.util.Try


case class AddService() extends  NFNService with Logging {
  override def toNFNName: NFNName = NFNName(Seq("AddService", "Int", "Int", "rInt"))

  def throwArgumentException(args: Seq[NFNServiceValue]) = throw new NFNServiceArgumentException(s"$toNFNName requires to arguments of type NFNIntValue and not $args")

  override def verifyArgs(args: Seq[NFNServiceValue]): Try[Seq[NFNServiceValue]] = {
    args match {
      case Seq(v1: NFNIntValue, v2: NFNIntValue) => Try(args)
      case _ => throwArgumentException(args)
    }
  }

  override def function: (Seq[NFNServiceValue]) => NFNServiceValue = {
    (values: Seq[NFNServiceValue]) =>
      values match {
        case Seq(l: NFNIntValue, r: NFNIntValue) => {
          NFNIntValue(l.amount + r.amount)
        }
        case _ => throwArgumentException(values)
      }
  }
}
