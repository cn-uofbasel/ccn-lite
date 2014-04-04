package nfn

import lambdacalculus.machine.{ConstValue, Value, CallExecutor}
import nfn.service._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.ActorRef
import scala.concurrent.Await
import scala.concurrent.duration._

case class LocalNFNCallExecutor(ccnWorker: ActorRef) extends CallExecutor {
  override def executeCall(call: String): Value = {

    val futValue =
    for {
      callableServ <- NFNService.parseAndFindFromName(call, ccnWorker)
    } yield {
      callableServ.exec match {
        case NFNIntValue(n) => ConstValue(n)
        case res @ _ => throw new Exception(s"LocalNFNCallExecutor: Result of executeCall is not implemented for: $res")
      }
    }
    Await.result(futValue, 20 seconds)
  }
}
