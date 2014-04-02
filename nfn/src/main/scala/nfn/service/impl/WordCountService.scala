package nfn.service.impl

import nfn.service._
import scala.util.Try
import nfn.service.NFNNameValue
import nfn.service.NFNName
import nfn.service.NFNIntValue
import nfn.service.CallableNFNService
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.ActorRef

/**
 * Created by basil on 24/03/14.
 */
case class WordCountService() extends NFNService {

  def countWords(doc: NFNName) = 42

  def throwException(args: Seq[NFNServiceValue]) = throw new NFNServiceArgumentException(s"$nfnName can only be applied to values of type NFNBinaryDataValue and not $args")

  override def verifyArgs(args: Seq[NFNServiceValue]): Try[Seq[NFNServiceValue]] = {
    if(args.forall(_.isInstanceOf[NFNBinaryDataValue])) Try(args)
    else throwException(args)
  }

  override def function: (Seq[NFNServiceValue]) => NFNServiceValue = { docs =>
    NFNIntValue(
      docs.map({
        case doc: NFNBinaryDataValue => new String(doc.data).split(" ").size
        case _ => throwException(docs)
      }).sum
    )
  }

  override def pinned: Boolean = false
}
