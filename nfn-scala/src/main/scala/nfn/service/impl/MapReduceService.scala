package nfn.service.impl

import scala.util.Try

import nfn.service._
import ccn.packet.Content
import akka.actor.ActorRef


/**
 * The map service is a generic service which transforms n [[NFNValue]] into a [[NFNListValue]] where each value was applied by a given other service of type [[NFNServiceValue]].
 * The first element of the arguments must be a [[NFNServiceValue]] and remaining n arguments must be a [[NFNListValue]].
 * The result of service invocation is a [[NFNListValue]].
 */
class MapService() extends NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = {
    (values: Seq[NFNValue], nfnMaster) => {
      values match {
        case Seq(NFNBinaryDataValue(servName, servData), args @ _*) => {
          val tryExec = NFNService.serviceFromContent(Content(servName, servData)) map { (serv: NFNService) =>
            NFNListValue(
              (args map { arg =>
                val execTime = serv.executionTimeEstimate flatMap { _ => this.executionTimeEstimate }
                serv.instantiateCallable(serv.ccnName, Seq(arg), nfnMaster, execTime).get.exec
              }).toList
            )
          }
          tryExec.get
        }
        case _ =>
          throw new NFNServiceArgumentException(s"A Map service must match Seq(NFNServiceValue, NFNValue*), but it was: $values ")
      }
    }
  }
}

/**
 * The reduce service is a generic service which transforms a [[NFNListValue]] into a single [[NFNValue]] with another given service.
 * The first element of the arguments must be a [[NFNServiceValue]] and the second argument must be a [[NFNListValue]].
 * The result of service invocation is a [[NFNValue]].
 */
class ReduceService() extends NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = {
    (values: Seq[NFNValue], nfnMaster) => {
      values match {
        case Seq(fun: NFNServiceValue, argList: NFNListValue) => {
          // TODO exec time
          fun.serv.instantiateCallable(fun.serv.ccnName, argList.values, nfnMaster, None).get.exec
        }
        case Seq(NFNBinaryDataValue(servName, servData), args @ _*) => {
          val tryExec: Try[NFNValue] = NFNService.serviceFromContent(Content(servName, servData)) flatMap {
            (serv: NFNService) =>
              // TODO exec time
              serv.instantiateCallable(serv.ccnName, args, nfnMaster, None) map {
                callableServ =>
                  callableServ.exec
              }
          }
          tryExec.get
        }
        case _ =>
          throw new NFNServiceArgumentException(s"A Reduce service must match Seq(NFNServiceValue, NFNListValue), but it was: $values")
      }
    }
  }
}
