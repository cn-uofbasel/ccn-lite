package nfn

import akka.actor.{ActorRef, Props, ActorSystem}
import akka.testkit.{TestActorRef, ImplicitSender, TestKit}
import org.scalatest.{BeforeAndAfterAll, Matchers, WordSpecLike}
import java.net.InetSocketAddress
import network.UDPConnection
import ccn.packet._
import nfn.NFNMaster.{ComputeResult, CCNAddToCache, CCNSendReceive}
import nfn.service.{NFNServiceLibrary, ContentStore}
import nfn.service.impl.AddService

class NFNMasterSpec(_system: ActorSystem) extends TestKit(_system) with ImplicitSender
with WordSpecLike with Matchers with BeforeAndAfterAll {


//  val nfnMasterLocal = system.actorOf(Props(NFNMasterLocal()))
  val nfnMasterLocalRef = TestActorRef[NFNMasterLocal]
  val nfnMasterLocalInstance = nfnMasterLocalRef.underlyingActor
//  val sock1 = new InetSocketAddress(9010)
//  val sock2 = new InetSocketAddress(9011)
  val name = Seq("test", "data")
  val data = "test".getBytes
  val interest = Interest(name)
  val content = Content(name, data)


  def this() = this(ActorSystem("NFNMasterSpec"))

  override def beforeAll {
    nfnMasterLocalRef ! CCNAddToCache(content)
    NFNServiceLibrary.nfnPublish(nfnMasterLocalRef)
    // TODO maybe make better
    while(nfnMasterLocalInstance.cs.find(content.name) == None) Thread.sleep(10)
  }

  testNFNMaster(nfnMasterLocalRef, "NFNMasterLocal")

  override def afterAll {
    TestKit.shutdownActorSystem(system)
  }


  def testNFNMaster(nfnMaster: ActorRef, nfnMasterName: String) = {
    def testComputeRequest(req: String, result: String) = {
      s"compute result '$result' for nfn request '$req'" in {
        val computeReqName = Seq(req, "NFN")
        nfnMaster ! CCNSendReceive(Interest(computeReqName))
        val actualContent = expectMsgType[Content]
        actualContent.name shouldBe computeReqName
        actualContent.data shouldBe result.getBytes
      }
    }

    s"An $nfnMasterName actor" should {
      "send interest and receive corresponding data" in {
        nfnMasterLocalInstance.cs.find(name) shouldBe Some(content)
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
        nfnMasterLocalInstance.cs.find(contentName) shouldBe Some(cacheContent)
      }
      testComputeRequest("1 ADD 2", "3")
      testComputeRequest(s"call 3 ${AddService().nfnName.toString} 12 30", "42")
      testComputeRequest(s"1 ADD call 3 ${AddService().nfnName.toString} 12 30", "43")
    }
  }
}

