package ccn

import network._
import akka.actor.{ActorSystem, Props}

object CCNNetwork {

  def apply() = {
    val system = ActorSystem("system")

    val leftActor = system.actorOf(Props[CCNClientFace](new CCNClientFace("LEFT")))
    val rightActor = system.actorOf(Props[CCNClientFace](new CCNClientFace("RIGHT")))

    val router = new CCNRouter("router")
    val routerActor = system.actorOf(Props[CCNRouterFace](new CCNRouterFace("ROUTER", router)))

    leftActor ! Connection(routerActor)
    routerActor ! Connection(leftActor)

    routerActor ! Connection(rightActor)
    rightActor ! Connection(routerActor)

    leftActor ! Send(Interest(ContentNameImpl("name")))
  }
}
