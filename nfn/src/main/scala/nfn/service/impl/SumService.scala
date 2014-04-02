package nfn.service.impl

import nfn.service._
import scala.util.Try
import nfn.service.NFNName
import nfn.service.NFNIntValue
import nfn.service.CallableNFNService
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.ActorRef

/**
 * Created by basil on 24/03/14.
 */
//case class SumService() extends  NFNService {
//  override def parse(unparsedName: String, unparsedValues: Seq[String], ccnWorker: ActorRef): Future[CallableNFNService] = {
//    val values = unparsedValues.map( (unparsedValue: String) => NFNIntValue(unparsedValue.toInt))
//    val name = NFNName(Seq(unparsedName))
//    assert(name == this.toNFNName)
//
//    val function = { (values: Seq[NFNServiceValue]) =>
//      Future(NFNIntValue(
//        values.map({
//          case NFNIntValue(i) => i
//          case _ => throw new NFNServiceException(s"${this.toNFNName} can only be applied to a single NFNIntValue and not $values")
//        }).sum
//      ))
//    }
//    Future(CallableNFNService(name, values, function))
//  }
//}
