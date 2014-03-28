import akka.actor._

import akka.pattern._

import akka.actor.Actor.Receive
import akka.util.Timeout
import bytecode.BytecodeLoader
import ccn.ccnlite.CCNLite
import ccn.packet.Interest
import ccn.packet.{Content, Interest}
import com.typesafe.config.ConfigFactory
import lambdacalculus.parser.LambdaParser
import network._
import nfn.CCNWorker
import nfn.CCNWorker.CCNSendReceive
import nfn.service.impl.AddService
import nfn.service.{NFNName, NFNServiceLibrary}

import LambdaMacros._
import language.experimental.macros
import nfn.{CCNWorker, ComputeWorker}
import scala.concurrent._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global



object NFNMain extends App {


  val config = ConfigFactory.parseString("""
    |akka.loglevel=DEBUG
    |akka.debug.lifecycle=on
    |akka.debug.receive=on
    |akka.debug.event-stream=on
    |akka.debug.unhandled=on
    """.stripMargin
  )

  val ccnIf = CCNLite
  val system: ActorSystem = ActorSystem("NFNActorSystem", config)

  val ccnWorker = system.actorOf(Props[CCNWorker], name = "ccnworker")


  Thread.sleep(2000)
  NFNServiceLibrary.nfnPublish(ccnWorker)
  Thread.sleep(2000)

  val interest = Interest(Seq("call 3 /AddService/Int/Int/rInt 10 2", "NFN"))

  implicit val timeout = Timeout(20 seconds)
  val content = Await.result((ccnWorker ? CCNSendReceive(interest)).mapTo[Content], timeout.duration)
  println(s"RESULT: $content")



  def repl(nfnSocket: ActorRef) = {
    val parser = new LambdaParser()
    step
    def step: Unit = {
      readLine("> ") match {
        case "exit" | "quit" | "q" =>
        case input @ _ => {
          parser.parse(input)
          val interest = Interest(Seq(input, "NFN"))
          (ccnWorker ? CCNSendReceive(interest)).mapTo[Content] onSuccess {
            case content => println(s"RESULT: ${interest.name.mkString("/")} -> '${new String(content.data)}")
          }
          step
        }
      }
    }
  }
}
