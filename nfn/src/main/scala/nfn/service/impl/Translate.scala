package nfn.service.impl

import nfn.service.{NFNBinaryDataValue, NFNServiceArgumentException, NFNValue, NFNService}
import scala.util.Try
import akka.actor.ActorRef

/**
 * Created by basil on 14/05/14.
 */
case class Translate() extends NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (args, nfnServer) =>
    args match {
      case Seq(NFNBinaryDataValue(name, data)) =>
        // translating should happen here
        val str = new String(data)
        println(s"TRANSLATE: $str")
        NFNBinaryDataValue(name, ((str + " ")*3).getBytes)
      case _ => throw argumentException(args)
    }
  }

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException = {
    new NFNServiceArgumentException(s"Translate service requires a single CCNName as a argument and not $args")
  }

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(NFNBinaryDataValue(_, _)) => Try(args)
      case _ => throw argumentException(args)
    }
  }
}
