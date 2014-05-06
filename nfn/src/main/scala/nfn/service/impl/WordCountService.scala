package nfn.service.impl

import scala.util.Try
import nfn.service._

/**
 * Created by basil on 24/03/14.
 */
case class WordCountService() extends NFNService {

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    if(args.forall(_.isInstanceOf[NFNBinaryDataValue])) Try(args)
    else throw argumentException(args)
  }

  override def function: (Seq[NFNValue]) => NFNValue = { docs =>
    NFNIntValue(
      docs.map({
        case doc: NFNBinaryDataValue => new String(doc.data).split(" ").size
        case _ => throw argumentException(docs)
      }).sum
    )
  }

  override def pinned: Boolean = false

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$ccnName can only be applied to values of type NFNBinaryDataValue and not $args")

}
