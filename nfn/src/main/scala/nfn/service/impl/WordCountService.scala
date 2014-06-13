package nfn.service.impl

import scala.util.Try
import nfn.service._
import akka.actor.ActorRef

class WordCountService() extends NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (docs, _) =>
    Thread.sleep(500)
    NFNIntValue(
      docs.map({
        case doc: NFNBinaryDataValue => new String(doc.data).split(" ").size
        case _ =>
          throw new NFNServiceArgumentException(s"$ccnName can only be applied to values of type NFNBinaryDataValue and not $docs")
      }).sum
    )
  }
}
