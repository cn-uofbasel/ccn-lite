import akka.actor._

import akka.pattern._

import akka.util.Timeout
import ccn.ccnlite.CCNLite
import ccn.packet.{Content, Interest}
import com.typesafe.config.ConfigFactory
import nfn.NFNMaster.CCNSendReceive
import nfn.service.impl._
import nfn.service._

import language.experimental.macros
import nfn._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import scala.util.{Success, Failure}


object NFNMain extends App {

  implicit val timeout = Timeout(14 seconds)

  val config = ConfigFactory.parseString("""
                                           |akka.loglevel=DEBUG
                                           |akka.debug.lifecycle=on
                                           |akka.debug.receive=on
                                           |akka.debug.event-stream=on
                                           |akka.debug.unhandled=on
                                           |akka.log-dead-letters=10
                                           |akka.log-dead-letters-during-shutdown=on
                                         """.stripMargin
  )

  val ccnIf = CCNLite
  val system: ActorSystem = ActorSystem("NFNActorSystem", config)

  val nfnWorker = system.actorOf(Props[NFNMasterLocal], name = "nfnmaster-network")
//  val nfnWorker = system.actorOf(Props[NFNMasterLocal], name = "nfnmaster-local")

  // Has to wait until udp connection is ready
  Thread.sleep(1000)

  val docdata1 = "one two three".getBytes
  val docdata2 = "one two three four".getBytes
  val ts = System.currentTimeMillis()
  val docname1 = Seq(s"$ts", "doc", "test", "1")
  val docname2 = Seq(s"$ts", "doc", "test", "2")
  val docContent1 = Content(docname1, docdata1)
  val docContent2 = Content(docname2, docdata2)

  nfnWorker ! NFNMaster.CCNAddToCache(docContent1)
  nfnWorker ! NFNMaster.CCNAddToCache(docContent2)
  NFNServiceLibrary.nfnPublish(nfnWorker)
  // Make sure that services and content is added to the nfn cache
  Thread.sleep(200)

//  (nfnWorker ? CCNSendReceive(Interest(docname1))).mapTo[Content] onComplete {
//    case Success(content) => println(s"DOC1: $content")
//    case Failure(e) => throw e
//  }


//  val addInterest = Interest(Seq(s"1 ADD 3", "NFN"))
//  //  val interest = Interest(Seq(s"call 3 ${AddService().nfnName.toString} 9 3", "NFN"))
//  (nfnWorker ? CCNSendReceive(addInterest)).mapTo[Content] onComplete  {
//    case Success(content) => println(s"RESULT ADD: $content")
//    case Failure(e) => throw e
//  }
//
//  val wcInterest = Interest(Seq(s"call 3 ${WordCountService().nfnName.toString} ${docname1.mkString("/", "/", "")} ${docname2.mkString("/", "/", "")}", "NFN"))
//  (nfnWorker ? CCNSendReceive(wcInterest)).mapTo[Content] onComplete  {
//    case Success(content) => println(s"RESULT WC: $content")
//    case Failure(e) => throw e
//  }
//
  val mapService = MapService().nfnName.toString
  val wordCountService = WordCountService().nfnName.toString

  val doc1 = NFNName(docname1).toString
  val doc2 = NFNName(docname2).toString

  val reduceService = ReduceService().nfnName.toString
  val sumService = SumService().nfnName.toString

  val mrInterest = Interest(Seq( s"call 4 $reduceService $sumService (call 4 $mapService $wordCountService $doc1 $doc2)", "NFN"))
//  val mrInterest = Interest(Seq( s"call 4 $mapService $wordCountService $doc1 $doc2", "NFN"))
  (nfnWorker ? CCNSendReceive(mrInterest)).mapTo[Content] onComplete  {
    case Success(content) => println(s"WC: $content")
    case Failure(e) => throw e
  }

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
