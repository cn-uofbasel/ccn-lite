import akka.actor._

import akka.actor.Actor.Receive
import bytecode.BytecodeLoader
import ccn.ccnlite.CCNLite
import ccn.packet.Interest
import lambdacalculus.parser.LambdaParser
import network._
import nfn.service.impl.AddService
import nfn.service.NFNServiceLibrary

import LambdaMacros._
import language.experimental.macros
import nfn.NFNWorker


object NFNMain extends App {
    val ccnIf = CCNLite
    val system: ActorSystem = ActorSystem("NFNActorSystem")

    val worker = system.actorOf(Props[NFNWorker], name = "NFNWorker")
    val nfnSocket = system.actorOf(Props(new UDPConnection(worker)), name = "nfnSocket")

    val interest = Interest(Seq("add 1 1", "NFN"))
    val binaryInterest = ccnIf.mkBinaryInterest(interest)
    Thread.sleep(2000)

    NFNServiceLibrary.nfnPublish(nfnSocket)
    Thread.sleep(2000)

//    repl(nfnSocket)

    system.shutdown

    def repl(nfnSocket: ActorRef) = {
      val parser = new LambdaParser()
      step
      def step: Unit = {
        readLine("> ") match {
          case "exit" | "quit" | "q" =>
          case input @ _ => {
            parser.parse(input)
            val interest = Interest(Seq(input, "NFN"))
            val binaryInterest = ccnIf.mkBinaryInterest(interest)
            nfnSocket ! Send(binaryInterest)
            step
          }
        }
      }
    }
}

