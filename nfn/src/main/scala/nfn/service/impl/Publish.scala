package nfn.service.impl

import nfn.service._
import scala.util.Try
import akka.actor.ActorRef
import akka.pattern._
import nfn.service.NFNServiceArgumentException
import nfn.service.NFNStringValue
import ccn.packet.{AddToCache, Content, CCNName, Interest}
import scala.concurrent.{Future, Await}
import scala.concurrent.duration._
import nfn.NFNApi
import akka.util.Timeout
import scala.concurrent.ExecutionContext.Implicits.global

/**
 * Created by basil on 08/05/14.
 */
case class Publish() extends NFNService {
  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (args, nfnMaster) =>
    println(s"$this was called")
    args match {
      case Seq(NFNBinaryDataValue(contentName, contentData), NFNBinaryDataValue(_, publishPrefixNameData)) => {
        implicit val timeout =  Timeout(5 seconds)

        val nameOfContentWithoutPrefixToAdd = CCNName(new String(publishPrefixNameData).split("/").tail:_*)
        println(s"BLA: $nameOfContentWithoutPrefixToAdd")
        nfnMaster ! NFNApi.AddToLocalCache(Content(nameOfContentWithoutPrefixToAdd, contentData), prependLocalPrefix = true)
        NFNEmptValue()
      }
      case _ => throw argumentException(args)
    }
  }

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException = {
    new NFNServiceArgumentException(s"$ccnName can only be applied to arguments of type CCNNameValue and not: $args")
  }

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(_:NFNBinaryDataValue, _:NFNBinaryDataValue) => Try(args)
      case _ => throw argumentException(args)
    }
  }
}
