package evaluation

import scala.util._
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.ActorSystem
import akka.pattern._
import akka.util.Timeout
import config.AkkaConfig

import nfn._
import ccn.packet._
import nfn.NFNMaster._


object TwoNodeEnv extends App {

  val timeoutDuration: FiniteDuration = 6 seconds
  implicit val timeout = Timeout( timeoutDuration)

  val node1Config = NodeConfig("localhost", 10000, 10001, "node1")
  val node2Config = NodeConfig("localhost", 11000, 11001, "node2")

  val docname1 = Seq(node1Config.prefix, "doc", "test1")
  val docdata1 = "one two three".getBytes
  val docContent1 = Content(docname1, docdata1)

  val docname2 = Seq(node2Config.prefix, "doc", "test2")
  val docdata2 = "one two three four".getBytes
  val docContent2 = Content(docname2, docdata2)

  val system1 = ActorSystem("NFNActorSystem1", AkkaConfig())
  val system2 = ActorSystem("NFNActorSystem2", AkkaConfig())

  // Make sure the ccnlite nodes communicate between each other
  val nfnWorker1 = NFNMasterFactory.network(system1, node1Config)
  val nfnWorker2 = NFNMasterFactory.network(system2, node2Config)
  println("Initialized nodes!")

  nfnWorker1 ! Connect(node2Config)
  println("Connected nodes!")

  nfnWorker1 ! CCNAddToCache(docContent1)
  nfnWorker2 ! CCNAddToCache(docContent2)
  println("Initialized caches!")

  println("Asking for content!")
  (nfnWorker1 ? CCNSendReceive(Interest(docname2))).mapTo[Content] onComplete {
    case Success(content) => println(s"RECV FROM REMOTE: $content")
    case Failure(error) => throw error
  }

  Thread.sleep(timeoutDuration.toMillis + 100)


  system1.shutdown()
  system2.shutdown()
  nfnWorker1 ! Exit()
  nfnWorker2 ! Exit()
}

