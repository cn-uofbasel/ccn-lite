package nfn.service.impl

import akka.actor.ActorRef
import nfn.service.{NFNServiceArgumentException, _}


class AddService() extends  NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (values: Seq[NFNValue], _) =>
    values match {
      case Seq(l: NFNIntValue, r: NFNIntValue) => {
        NFNIntValue(l.amount + r.amount)
      }
      case _ =>
        throw new NFNServiceArgumentException(s"$ccnName requires to arguments of type NFNIntValue and not $values")
    }
  }
}
