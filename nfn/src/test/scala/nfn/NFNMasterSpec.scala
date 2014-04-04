package nfn

import akka.actor.{ActorRef, Props, ActorSystem}
import akka.testkit.{TestActorRef, ImplicitSender, TestKit}
import org.scalatest.{BeforeAndAfterAll, Matchers, WordSpecLike}
import java.net.InetSocketAddress
import network.UDPConnection
import ccn.packet._
import nfn.NFNMaster.{ComputeResult, CCNAddToCache, CCNSendReceive}
import nfn.service.{NFNServiceLibrary, ContentStore}
import nfn.service.impl.{WordCountService, AddService}
import lambdacalculus.LambdaCalculus
import lambdacalculus.parser.ast.{LambdaNFNPrinter, LambdaPrettyPrinter}

class NFNMasterSpec(_system: ActorSystem) extends TestKit(_system) with ImplicitSender
with WordSpecLike with Matchers with BeforeAndAfterAll {

  val nfnMasterLocalRef = TestActorRef[NFNMasterLocal]
  val nfnMasterLocalInstance = nfnMasterLocalRef.underlyingActor

  val nfnMasterNetworkRef = TestActorRef[NFNMasterNetwork]
  val nfnMasterNetworkInstance = nfnMasterNetworkRef.underlyingActor

//  val sock1 = new InetSocketAddress(9010)
//  val sock2 = new InetSocketAddress(9011)

  val name = Seq("test", "data")

  val doc1Name = Seq("test", "doc", "1")
  val doc2Name = Seq("test", "doc", "2")

  val doc1Content = Content(doc1Name, "one two".getBytes)
  val doc2Content = Content(doc2Name, "one two three".getBytes)

  val data = "test".getBytes
  val interest = Interest(name)
  val content = Content(name, data)

  val lc = LambdaCalculus()


  def this() = this(ActorSystem("NFNMasterSpec"))

  override def beforeAll {
    def initMaster(ref: ActorRef) = {
      ref ! CCNAddToCache(content)
      ref ! CCNAddToCache(doc1Content)
      ref ! CCNAddToCache(doc2Content)
      NFNServiceLibrary.nfnPublish(ref)
    }
    initMaster(nfnMasterNetworkRef)
    initMaster(nfnMasterLocalRef)
    while(nfnMasterLocalInstance.cs.find(content.name) == None) Thread.sleep(10)
  }

  testNFNMaster(nfnMasterLocalRef, "NFNMasterLocal")
  testNFNMaster(nfnMasterNetworkRef, "NFNMasterNetwork", nfnNetwork = true)

  override def afterAll {
    TestKit.shutdownActorSystem(system)
  }


  def testNFNMaster(nfnMasterRef: ActorRef, nfnMasterName: String, nfnNetwork: Boolean = false) = {
    def testComputeRequest(req: String, result: String) = {
      val parsedExpr = lc.parse(req).get
      val parsedReq = if(nfnNetwork) LambdaNFNPrinter(parsedExpr) else LambdaPrettyPrinter(parsedExpr)

      val temporaryPaddedResult = if(nfnNetwork) {"RST|" + result } else { result }

      s"compute result '$temporaryPaddedResult' for ${if(nfnNetwork) "local" else "nfn"} request '$parsedReq'" in {

        val computeReqName = Seq(parsedReq, "NFN")
        nfnMasterRef ! CCNSendReceive(Interest(computeReqName))
        val actualContent = expectMsgType[Content]
        actualContent.name shouldBe computeReqName
        actualContent.data shouldBe temporaryPaddedResult.getBytes
      }
    }

    s"An $nfnMasterName actor" should {
      "send interest and receive corresponding data" in {
        nfnMasterLocalInstance.cs.find(name) shouldBe Some(content)
        nfnMasterRef ! CCNSendReceive(interest)
        val actualContent = expectMsg(content)
        actualContent.data shouldBe data
      }

      "add content to cache" in {
        val contentName = Seq("name", "addtocache")
        val contentData = "added to cache!".getBytes
        val cacheContent = Content(contentName, contentData)
        nfnMasterRef ! CCNAddToCache(cacheContent)
        // TODO maybe make this nicer?
        Thread.sleep(10)
        nfnMasterRef ! CCNSendReceive(Interest(contentName))
        expectMsg(cacheContent)
      }
      testComputeRequest("1 ADD 2", "3")
      testComputeRequest(s"call 3 ${AddService().nfnName.toString} 12 30", "42")
      testComputeRequest(s"1 ADD call 3 ${AddService().nfnName.toString} 12 30", "43")
      testComputeRequest(s"call 3 ${WordCountService().nfnName.toString} ${doc1Name.mkString("/", "/", "")} ${doc2Name.mkString("/", "/", "")}", "5")
      testComputeRequest(s"call 1 ${WordCountService().nfnName.toString}", "0")
      testComputeRequest(s"call 0 ${WordCountService().nfnName.toString}", "0")
    }
  }
}

