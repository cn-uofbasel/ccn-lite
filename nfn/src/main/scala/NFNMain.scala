import akka.actor._

import ccn.ccnlite.CCNLite
import ccn.packet.Interest
import lambdacalculus.parser.LambdaParser
import network._
import nfn.service.NFNServiceLibrary

import LambdaMacros._
import language.experimental.macros
import nfn.NFNWorker

/**
 * Created by basil on 24/03/14.
 */
object NFNMain extends App {
  val ccnIf = CCNLite
  val system = ActorSystem("NFNActorSystem")

  val worker = system.actorOf(Props[NFNWorker], name = "NFNWorker")
  val nfnSocket = system.actorOf(Props(new UDPConnection(worker)), name = "nfnSocket")

//  val interest = Interest(Seq("call 3 /AddService/Int/Int/rInt 11 24", "NFN"))
  val interest = Interest(Seq("add 1 1", "NFN"))
  val binaryInterest = ccnIf.mkBinaryInterest(interest)

  Thread.sleep(2000)
  NFNServiceLibrary.nfnPublish(nfnSocket)

  val parser = new LambdaParser()

  firstStep

  system.shutdown


  def firstStep = step

  private def step(): Unit = {
    readLine("> ") match {
      case "exit" | "quit" | "q" =>
      case input @ _ => {
        parser.parse(input)
        val interest = Interest(Seq(input, "NFN"))
        val binaryInterest = ccnIf.mkBinaryInterest(interest)
        nfnSocket ! Send(binaryInterest)
        step()
      }
    }

  }
}

