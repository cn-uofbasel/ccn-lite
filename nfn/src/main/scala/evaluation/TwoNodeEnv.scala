package evaluation

import scala.util._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.{Props, ActorSystem}
import akka.pattern._
import akka.util.Timeout
import config.AkkaConfig

import nfn._
import ccn.packet._
import nfn.NFNMaster._

object TwoNodeEnv extends App {

  implicit val timeout = Timeout(20 seconds)

  val system1: ActorSystem = ActorSystem("NFNActorSystem1", AkkaConfig())
  val system2: ActorSystem = ActorSystem("NFNActorSystem2", AkkaConfig())

  // Make sure the ccnlite nodes communicate between each other
  val nfnWorker1 = NFNMasterFactory.network(system1, "localhost", 10000, 10001)
  val nfnWorker2 = NFNMasterFactory.network(system2, "localhost", 11000, 11001)

  Thread.sleep(1000)
  val docdata1 = "one two three".getBytes
  val docdata2 = "one two three four".getBytes
  val ts = System.currentTimeMillis()
  val docname1 = Seq(s"$ts", "doc", "test", "1")
  val docname2 = Seq(s"$ts", "doc", "test", "2")
  val docContent1 = Content(docname1, docdata1)
  val docContent2 = Content(docname2, docdata2)

  nfnWorker2 ! CCNAddToCache(docContent1)

  (nfnWorker1 ? CCNSendReceive(Interest(docname1))).mapTo[Content] onComplete {
    case Success(content) => println(s"RECV: $content")
    case Failure(error) => throw error
  }

  system1.shutdown()
  Thread.sleep(20000)
  nfnWorker1 ! Exit()
  nfnWorker2 ! Exit()
}

