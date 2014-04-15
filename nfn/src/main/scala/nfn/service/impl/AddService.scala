package nfn.service.impl

import nfn.service._
import com.typesafe.scalalogging.slf4j.Logging
import scala.util.Try
import nfn.service.NFNServiceArgumentException


case class AddService() extends  NFNService with Logging {

  def argumentException(args: Seq[NFNValue]):NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$nfnName requires to arguments of type NFNIntValue and not $args")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(v1: NFNIntValue, v2: NFNIntValue) => Try(args)
      case _ => throw argumentException(args)
    }
  }

  override def function: (Seq[NFNValue]) => NFNValue = { (values: Seq[NFNValue]) =>
    values match {
      case Seq(l: NFNIntValue, r: NFNIntValue) => {
        logger.info(s"L: $l R: $r")
        NFNIntValue(l.amount + r.amount)
      }
      case _ => throw argumentException(values)
    }
  }
}
