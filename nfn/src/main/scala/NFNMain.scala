import akka.actor._

import akka.pattern._

import akka.util.Timeout
import ccn.packet.{NFNInterest, CCNName, Content, Interest}
import com.typesafe.config.ConfigFactory
import nfn.NFNMaster._
import nfn.service.impl._
import nfn.service._

import language.experimental.macros
import nfn._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import scala.util.{Success, Failure}


object NFNMain extends App {


  val timeoutDuration: FiniteDuration = 14 seconds
  implicit val timeout = Timeout.apply(timeoutDuration)

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

  val system: ActorSystem = ActorSystem("NFNActorSystem", config)

//  val nfnWorker = NFNMasterFactory.local(system)
  val nodeConfig = NodeConfig("localhost", 10000, 10001, "node")
  val nfnWorker = NFNMasterFactory.network(system, nodeConfig)

  // Has to wait until udp connection is ready
//  Thread.sleep(1000)

  val docdata1 = "one two three".getBytes
  val docdata2 = "one two three four".getBytes
  val ts = System.currentTimeMillis()
  val docname1 = CCNName(s"$ts", "doc", "test", "1")
  val docname2 = CCNName(s"$ts", "doc", "test", "2")
  val docContent1 = Content(docname1, docdata1)
  val docContent2 = Content(docname2, docdata2)

  nfnWorker ! NFNMaster.CCNAddToCache(docContent1)
  nfnWorker ! NFNMaster.CCNAddToCache(docContent2)
  NFNServiceLibrary.nfnPublish(nfnWorker)
  // Make sure that services and content is added to the nfn cache
  Thread.sleep(200)

  (nfnWorker ? CCNSendReceive(Interest(docname1))).mapTo[Content] onComplete {
    case Success(content) => println(s"DOC1: $content")
    case Failure(e) => throw e
  }


  val addInterest = NFNInterest(s"1 ADD 3")
  //  val interest = Interest(Seq(s"call 3 ${AddService().nfnName.toString} 9 3", "NFN"))
  (nfnWorker ? CCNSendReceive(addInterest)).mapTo[Content] onComplete  {
    case Success(content) => println(s"RESULT ADD: $content")
    case Failure(e) => throw e
  }

  val wcInterest = NFNInterest(s"call 3 ${WordCountService().ccnName.toString} $docname1 $docname2")
  (nfnWorker ? CCNSendReceive(wcInterest)).mapTo[Content] onComplete  {
    case Success(content) => println(s"RESULT WC: $content")
    case Failure(e) => throw e
  }
  Thread.sleep(timeoutDuration.toMillis)
  nfnWorker ! Exit()
  system.shutdown()
//
//  val mapService = MapService().nfnName.toString
//  val wordCountService = WordCountService().nfnName.toString
//
//  val doc1 = NFNName(docname1).toString
//  val doc2 = NFNName(docname2).toString
//
//  val reduceService = ReduceService().nfnName.toString
//  val sumService = SumService().nfnName.toString
//
//  val mrInterest = Interest(Seq( s"call 4 $reduceService $sumService (call 4 $mapService $wordCountService $doc1 $doc2)", "NFN"))
////  val mrInterest = Interest(Seq( s"call 4 $mapService $wordCountService $doc1 $doc2", "NFN"))
//  (nfnWorker ? CCNSendReceive(mrInterest)).mapTo[Content] onComplete  {
//    case Success(content) => println(s"WC: $content")
//    case Failure(e) => throw e
//  }

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
