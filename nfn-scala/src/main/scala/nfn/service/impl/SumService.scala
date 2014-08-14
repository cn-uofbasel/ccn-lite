package nfn.service.impl

import nfn.service._
import scala.util.Try
import nfn.service.NFNServiceArgumentException
import nfn.service.NFNIntValue
import akka.actor.ActorRef

/**
 * Simple service which takes a sequence of [[NFNIntValue]] and sums them to a single [[NFNIntValue]]
 */
class SumService() extends NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = {
    (values: Seq[NFNValue], _) => {
      NFNIntValue(
        values.map({
          case i: NFNIntValue => i.amount
          case _ => throw  new NFNServiceArgumentException(s"SumService requires a sequence of NFNIntValue's and not $values")
        }).sum
      )
    }
  }
}
