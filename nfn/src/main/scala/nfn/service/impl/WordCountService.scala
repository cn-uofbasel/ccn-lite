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

  override def parse(unparsedName: String, unparsedValues: Seq[String], ccnWorker: ActorRef): Future[CallableNFNService] = {
    val values = unparsedValues match {
      case Seq(docNameString) => Seq(
        NFNNameValue(NFNName(Seq(docNameString)))
      )
      case _ => throw new Exception(s"Service $toNFNName could not parse single Int value from: '$unparsedValues'")
    }
    val name = NFNName(Seq(unparsedName))
    assert(name == this.toNFNName)

    val function = { (values: Seq[NFNServiceValue]) =>
      values match {
        case Seq(docName: NFNNameValue) => { Future(
          NFNIntValue(countWords(docName.name))
        )}
        case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
      }

    }
    Future(CallableNFNService(name, values, function))
  }

  override def toNFNName: NFNName = NFNName(Seq("WordCountService/Int/rInt"))
}
