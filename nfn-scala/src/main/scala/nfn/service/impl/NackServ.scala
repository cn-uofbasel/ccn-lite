package nfn.service.impl

import akka.actor.ActorRef
import nfn.NFNServer
import nfn.service.{NFNServiceExecutionException, NFNValue, NFNService}

/**
 * Created by basil on 17/06/14.
 */
class NackServ extends NFNService {
  override def function: (Seq[NFNValue], ActorRef) => NFNValue = {
    throw new NFNServiceExecutionException("Provoking a nack")
  }
}
