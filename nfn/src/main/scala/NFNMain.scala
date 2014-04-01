import akka.actor._

import akka.pattern._

import akka.util.Timeout
import ccn.ccnlite.CCNLite
import ccn.packet.{Content, Interest}
import com.typesafe.config.ConfigFactory
import nfn.NFNWorker.CCNSendReceive
import nfn.service.{NFNName, NFNServiceLibrary}

import language.experimental.macros
import nfn.NFNWorker
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

  val ccnWorker = system.actorOf(Props[NFNWorker], name = "ccnworker")

  Thread.sleep(2000)
  val docdata1 = "This is a test document with 8 words! but 12 is better".getBytes
  val docdata2 = "This is the second document with as many words required to get a totally random number. No i have nothing better to do than to count words in this doc.".getBytes
  val docContent1 = Content(Seq("doc", "test", "1"), docdata1)
  val docContent2 = Content(Seq("doc", "test", "2"), docdata2)
  ccnWorker ! NFNWorker.CCNAddToCache(docContent1)
  ccnWorker ! NFNWorker.CCNAddToCache(docContent2)

  NFNServiceLibrary.nfnPublish(ccnWorker)
  Thread.sleep(2000)

  val interest = Interest(Seq(s"call 3 /WordCountService/NFNName/rInt /doc/test/1 /doc/test/2", "NFN"))
//  val interest = Interest(Seq(s"call 3 /AddService/Int/Int/rInt 9 1", "NFN"))
  implicit val timeout = Timeout(20 seconds)
  val content = Await.result((ccnWorker ? CCNSendReceive(interest)).mapTo[Content], timeout.duration)
  println(s"RESULT: $content")



//  def repl(nfnSocket: ActorRef) = {
//    val parser = new LambdaParser()
//    step
//    def step: Unit = {
//      readLine("> ") match {
//        case "exit" | "quit" | "q" =>
//        case input @ _ => {
//          parser.parse(input)
//          val interest = Interest(Seq(input, "NFN"))
//          (ccnWorker ? CCNSendReceive(interest)).mapTo[Content] onSuccess {
//            case content => println(s"RESULT: ${interest.name.mkString("/")} -> '${new String(content.data)}")
//          }
//          step
//        }
//      }
//    }
//  }
}
