package nfn

import lambdacalculus.machine.{ConstValue, Value, CallExecutor}
import nfn.service.{NFNIntValue, NFNService}
import scala.util.{Failure, Success}

/**
 * Created by basil on 24/03/14.
 */
case class LocalNFNCallExecutor() extends CallExecutor {
  override def executeCall(call: String): Value = {
    NFNService.parseAndFindFromName(call).flatMap({ serv => serv.exec}) match {
      case Success(NFNIntValue(n)) => ConstValue(n)
      case Success(res @ _) => throw new Exception(s"LocalNFNCallExecutor: Result of executeCall is not implemented for: $res")
      case Failure(e) => throw new Exception(e)
    }
  }
}
