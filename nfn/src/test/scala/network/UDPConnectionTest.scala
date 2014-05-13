package network

import org.scalatest.FlatSpec

import akka.actor.ActorSystem
import akka.actor.Actor
import akka.actor.Props
import akka.testkit.TestKit
import org.scalatest.WordSpecLike
import org.scalatest.Matchers
import org.scalatest.BeforeAndAfterAll
import akka.testkit.ImplicitSender
import java.net.InetSocketAddress
import network.UDPConnection.Send

//object MySpec {
//  class EchoActor extends Actor {
//    def receive = {
//      case x => sender ! x
//    }
//  }
//}

//class UDPConnectionSpec(_system: ActorSystem) extends TestKit(_system) with ImplicitSender
//with WordSpecLike with Matchers with BeforeAndAfterAll {
//
//
//  val sock1 = new InetSocketAddress(9010)
//  val sock2 = new InetSocketAddress(9011)
//  val data = "test".getBytes
//  def this() = this(ActorSystem("UDPConnectionSpec"))
//
//  override def afterAll {
//    TestKit.shutdownActorSystem(system)
//  }
//
//  "An Echo actor" must {
//
//    "send back messages unchanged" in {
//      val actualCon = system.actorOf(Props(new UDPConnection(localAbstractMachine = sock1, target = sock2)))
//      actualCon ! UDPConnection.Send(data)
//      expectMsg(data)
//    }
//
//  }
//}

