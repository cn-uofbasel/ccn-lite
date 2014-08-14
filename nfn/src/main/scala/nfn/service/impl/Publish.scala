package nfn.service.impl

import akka.actor.ActorRef
import ccn.packet.{CCNName, Content}
import config.AkkaConfig
import nfn.NFNApi
import nfn.service.{NFNServiceArgumentException, _}

class Publish() extends NFNService {
  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (args, nfnServer) =>
    args match {
      case Seq(NFNBinaryDataValue(contentName, contentData), NFNBinaryDataValue(_, publishPrefixNameData), _) => {
        val nameOfContentWithoutPrefixToAdd = CCNName(new String(publishPrefixNameData).split("/").tail:_*)
        nfnServer ! NFNApi.AddToLocalCache(Content(nameOfContentWithoutPrefixToAdd, contentData), prependLocalPrefix = true)
        NFNEmptyValue()
      }
      case _ =>
        throw new NFNServiceArgumentException(s"$ccnName can only be applied to arguments of type CCNNameValue and not: $args")
    }
  }
}
