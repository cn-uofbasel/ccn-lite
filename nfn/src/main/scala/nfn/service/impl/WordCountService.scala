package nfn.service.impl

import nfn.service._
import scala.util.Try
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import nfn.service._

/**
 * Created by basil on 24/03/14.
 */
case class WordCountService() extends NFNService {

  def countWords(doc: NFNName) = 42

  def throwException(args: Seq[NFNValue]) = throw new NFNServiceArgumentException(s"$nfnName can only be applied to values of type NFNBinaryDataValue and not $args")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    if(args.forall(_.isInstanceOf[NFNBinaryDataValue])) Try(args)
    else throwException(args)
  }

  override def function: (Seq[NFNValue]) => NFNValue = { docs =>
    NFNIntValue(
      docs.map({
        case doc: NFNBinaryDataValue => new String(doc.data).split(" ").size
        case _ => throwException(docs)
      }).sum
    )
  }

  override def pinned: Boolean = false
}
