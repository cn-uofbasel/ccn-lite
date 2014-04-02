package nfn

import akka.actor.{Props, ActorSystem}
import akka.testkit.{ImplicitSender, TestKit}
import org.scalatest.{BeforeAndAfterAll, Matchers, WordSpecLike}
import java.net.InetSocketAddress
import network.UDPConnection
import ccn.packet._
import nfn.NFNMaster.{CCNAddToCache, CCNSendReceive}
import nfn.service.ContentStore

class NFNMasterSpec(_system: ActorSystem) extends TestKit(_system) with ImplicitSender
with WordSpecLike with Matchers with BeforeAndAfterAll {


  val nfnMaster = system.actorOf(Props(NFNMasterLocal()))
//  val sock1 = new InetSocketAddress(9010)
//  val sock2 = new InetSocketAddress(9011)
  val name = Seq("test", "data")
  val data = "test".getBytes
  val interest = Interest(name)
  val content = Content(name, data)


  def this() = this(ActorSystem("NFNMasterSpec"))

  override def beforeAll {
    nfnMaster ! CCNAddToCache(content)
    while(ContentStore.find(name) == None) Thread.sleep(10)
  }

  override def afterAll {
    TestKit.shutdownActorSystem(system)
  }

  "An NFNMaster-local actor" should {
    "send interest and receive corresponding data" in {
      ContentStore.find(name) shouldBe Some(content)
      nfnMaster ! CCNSendReceive(interest)
      val expectedContent = expectMsg(content)
      expectedContent.data shouldBe data
    }

    "add content to cache" in {
      val contentName = Seq("name", "addtocache")
      val contentData = "added to cache!".getBytes
      val cacheContent = Content(contentName, contentData)
      nfnMaster ! CCNAddToCache(cacheContent)
      // TODO maybe make this nicer?
      Thread.sleep(10)
      ContentStore.find(contentName) shouldBe Some(cacheContent)
    }
  }
}

