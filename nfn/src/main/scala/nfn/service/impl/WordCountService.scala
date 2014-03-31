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

  override def instantiateCallable(name: NFNName, futValues: Seq[Future[NFNServiceValue]], ccnWorker: ActorRef): Future[CallableNFNService] = {
    assert(name == this.toNFNName)

    val function: (Seq[NFNServiceValue]) => NFNIntValue = {
      case Seq(doc: NFNBinaryDataValue) => {
        NFNIntValue(
          new String(doc.data).split(" ").size
        )
      }
      case values @ _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNBinaryDataValue and not $values")
    }

    Future.sequence(futValues) map { values =>
      CallableNFNService(name, values, function)
    }
  }

  override def toNFNName: NFNName = NFNName(Seq("/WordCountService/NFNName/rInt"))

}
