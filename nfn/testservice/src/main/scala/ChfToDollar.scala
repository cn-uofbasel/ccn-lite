import akka.actor.ActorRef
import nfn.service._
import scala.util.Try


case class ChfToDollar() extends NFNService {
  override def verifyArgs(args: Seq[NFNValue]): Try[Seq[NFNValue]] = {
    if(args.size == 1 && args.head.isInstanceOf[NFNIntValue]) Try(args)
    else throw argumentException(args)
  }

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (args, _) =>
    println("External service successfully loaded!")
    if(args.size == 1 && args.head.isInstanceOf[NFNIntValue]) {
      NFNIntValue(args.head.asInstanceOf[NFNIntValue].amount * 2)
    }
    else throw argumentException(args)
  }

  override def argumentException(args: Seq[NFNValue]): NFNServiceArgumentException =
    new NFNServiceArgumentException(s"$ccnName can only be applied a single value of type NFNIntValue and not $args")
}


