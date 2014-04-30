package nfn.service.impl

import scala.util.Try

import nfn.service._
import ccn.packet.Content


/**
 * The map service is a generic service which transforms n [[NFNValue]] into a [[NFNListValue]] where each value was applied by a given other service of type [[NFNServiceValue]].
 * The first element of the arguments must be a [[NFNServiceValue]] and remaining n arguments must be a [[NFNListValue]], which need to be verifiable by the mapper service.
 * The result of service invocation is a [[NFNListValue]].
 */
case class MapService() extends NFNService {
  def argumentException(args: Seq[NFNValue]) =
    new NFNServiceArgumentException(s"A Map service must match Seq(NFNServiceValue, NFNValue*), but it was: $args ")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(NFNBinaryDataValue(servName, servData), args@_*) => {
        // TODO a map function could take more than one arg!

        // Tries to get the service from content and then verifies all arguments with the tried service.
        // Since map assumes every map service only takes a single argument, we need to transform a Try[Seq[...]] by taking head on the sequence
        // and get if succes. This is safe because the whole thing is wrapped into a Try anyway, so if get fails it is packed into the final Try
        NFNService.serviceFromContent(Content(servName, servData)) map { (serv: NFNService) =>
          args map { arg =>
            (serv.verifyArgs(Seq(arg)) map { _.head }).get
          }
        }
      }
      case _ => throw argumentException(args)
    }
  }


  override def function: (Seq[NFNValue]) => NFNValue = {
    (values: Seq[NFNValue]) => {
      values match {
        case Seq(NFNBinaryDataValue(servName, servData), args @ _*) => {
          val tryExec = NFNService.serviceFromContent(Content(servName, servData)) map { (serv: NFNService) =>
            NFNListValue(
              (args map { arg =>
                serv.instantiateCallable(serv.ccnName, Seq(arg)).get.exec
              }).toList
            )
          }
          tryExec.get
        }
        case _ => throw argumentException(values)
      }
    }
  }
}

/**
 * The reduce service is a generic service which transforms a [[NFNListValue]] into a single [[NFNValue]] with another given service.
 * The first element of the arguments must be a [[NFNServiceValue]] and the second argument must be a [[NFNListValue]].
 * The result of service invocation is a [[NFNValue]].
 */
case class ReduceService() extends NFNService {

  def argumentException(args: Seq[NFNValue]) =
    new NFNServiceArgumentException(s"A Reduce service must match Seq(NFNServiceValue, NFNListValue), but it was: $args ")

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    args match {
      case Seq(NFNBinaryDataValue(servName, servData), args @ _*) =>
        NFNService.serviceFromContent(Content(servName, servData)) flatMap { (serv: NFNService) =>
          serv.verifyArgs(args)
        }
      case _ => throw argumentException(args)
    }
  }

  override def function: (Seq[NFNValue]) => NFNValue = {
    (values: Seq[NFNValue]) => {
      values match {
        case Seq(fun: NFNServiceValue, argList: NFNListValue) => {
          fun.serv.instantiateCallable(fun.serv.ccnName, argList.values).get.exec
        }
        case Seq(NFNBinaryDataValue(servName, servData), args @ _*) => {
          val tryExec: Try[NFNValue] = NFNService.serviceFromContent(Content(servName, servData)) flatMap {
            (serv: NFNService) =>
              serv.instantiateCallable(serv.ccnName, args) map {
                callableServ =>
                  callableServ.exec
              }
          }
          tryExec.get
        }
        case _ => throw argumentException(values)
      }
    }
  }
}
