package nfn.service.impl

import nfn.service._
import scala.util.Try
import nfn.service.NFNServiceArgumentException
import nfn.service.NFNIntValue

/**
 * Simple service which takes a sequence of [[NFNIntValue]] and sums them to a single [[NFNIntValue]]
 */
case class SumService() extends NFNService {

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    if(args.forall( _.isInstanceOf[NFNIntValue])) Try(args)
    else throw argumentException(args)
  }

  override def function: (Seq[NFNValue]) => NFNValue = {
    (values: Seq[NFNValue]) => {
      NFNIntValue(
        values.map({
          case i: NFNIntValue => i.amount
          case _ => throw argumentException(values)
        }).sum
      )
    }
  }

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException =
    new NFNServiceArgumentException(s"SumService requires a sequence of NFNIntValue's and not $args")

}
