package nfn.service.impl

import nfn.service._
import scala.util.Try

/**
 * Created by basil on 05/05/14.
 */
case class NameRegister() extends NFNService {

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException = {
    new NFNServiceArgumentException(s"$ccnName can only be applied to a single variable name and not: $args")
  }

  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    if(args.size == 1 && args.head.isInstanceOf[NFNStringValue]) {
      Try(args)
    } else {
      throw argumentException(args)
    }
  }

  override def function: (Seq[NFNValue]) => NFNValue = { args =>
    ???
  }
}
