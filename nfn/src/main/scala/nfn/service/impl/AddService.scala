package nfn.service.impl

import nfn.service._
import com.typesafe.scalalogging.slf4j.Logging
import scala.util.Try
import nfn.service.NFNServiceArgumentException
import akka.actor.ActorRef


case class AddService() extends  NFNService with Logging {

  def argumentException(args: Seq[NFNValue]):NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$ccnName requires to arguments of type NFNIntValue and not $args")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(v1: NFNIntValue, v2: NFNIntValue) => Try(args)
      case _ => throw argumentException(args)
    }
  }

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (values: Seq[NFNValue], _) =>
    values match {
      case Seq(l: NFNIntValue, r: NFNIntValue) => {
        NFNIntValue(l.amount + r.amount)
      }
      case _ => throw argumentException(values)
    }
  }
}
