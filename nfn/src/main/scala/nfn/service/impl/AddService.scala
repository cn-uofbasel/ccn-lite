package nfn.service.impl

import nfn.service._
import com.typesafe.scalalogging.slf4j.Logging
import scala.util.Try
import nfn.service.NFNName
import nfn.service.NFNIntValue
import nfn.service.CallableNFNService
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.ActorRef


case class AddService() extends  NFNService with Logging {
  override def instantiateCallable(name: NFNName, futValues: Seq[Future[NFNServiceValue]], ccnWorker: ActorRef): Future[CallableNFNService] = {
    val (futLValue, futRValue) = futValues match {
      case Seq(lval, rval) => (lval, rval)
      case _ => throw new Exception(s"Service $toNFNName can only be applied to two NFNIntValues and not '$futValues'")
    }

    assert(name == toNFNName, s"Service $toNFNName is created with wrong name $name")

    val function = { (values: Seq[NFNServiceValue]) =>
      values match {
        case Seq(l: NFNIntValue, r: NFNIntValue) => {
          NFNIntValue(l.amount + r.amount)
        }
        case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
      }
    }

    Future.sequence(futValues) map {
      case values @ Seq(lval: NFNIntValue, rval: NFNIntValue) => CallableNFNService(name, values, function)
      case _ => throw new Exception(s"Service $toNFNName can only be applied to two NFNIntValues and not '$futValues'")
    }
  }

  override def toNFNName: NFNName = NFNName(Seq("/AddService/Int/Int/rInt"))

  override def pinned: Boolean = false

}
