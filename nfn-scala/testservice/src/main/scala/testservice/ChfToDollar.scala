package testservice

import akka.actor.ActorRef
import nfn.service._


class OtherClass() {
  def foo(n: Int) = {
    println(s"OtherClass: $n")
  }
}

class ChfToDollar() extends NFNService {

  override def function: (Seq[NFNValue], ActorRef) => NFNValue = { (args, _) =>
    println("External service successfully loaded!")
    new OtherClass().foo(3)
    if(args.size == 1 && args.head.isInstanceOf[NFNIntValue]) {
      NFNIntValue(args.head.asInstanceOf[NFNIntValue].amount * 2)
    }
    else throw  new NFNServiceArgumentException(s"$ccnName can only be applied a single value of type NFNIntValue and not $args")
  }
}

object TestService extends App {
  println(new ChfToDollar())
}

