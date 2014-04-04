package nfn.service.impl

import scala.util.Try

import nfn.service._


object MapReduceService {

  def throwArgumentException(args: Seq[NFNValue]) =
    throw new NFNServiceArgumentException(s"A Map service must match Seq(NFNServiceValue, NFNValue*), but it was: $args " +
                                           "(MapService only supports services that take a single argument)")

  case class MapService() extends NFNService {
    override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
      args match {
        case Seq(fun: NFNServiceValue, args@_*) => {
          // TODO a map function could take more than one arg!
          args.foreach { arg => fun.serv.verifyArgs(Seq(arg))}
          Try(args)
        }
        case _ => throwArgumentException(args)
      }
    }


    override def function: (Seq[NFNValue]) => NFNValue = {
      (values: Seq[NFNValue]) => {
        values match {
          case Seq(fun: NFNServiceValue, args@_*) => {
            NFNListValue(
              args map { arg =>
                fun.serv.instantiateCallable(fun.serv.nfnName, Seq(arg)).get.exec
              }
            )
          }
          case _ => throwArgumentException(values)
        }
      }
    }
  }

  case class ReduceService() extends NFNService {
    override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
      args match {
        case Seq(fun: NFNServiceValue, argList:NFNListValue) => {
          fun.serv.verifyArgs(argList.values)
          Try(args)
        }
        case _ => throwArgumentException(args)
      }
    }

    override def function: (Seq[NFNValue]) => NFNValue = {
      (values: Seq[NFNValue]) => {
        values match {
          case Seq(fun: NFNServiceValue, argList: NFNListValue) => {
            fun.serv.instantiateCallable(fun.serv.nfnName, argList.values).get.exec
          }
          case _ => throwArgumentException(values)
        }
      }
    }
  }
}
