package nfn

import akka.actor._
import akka.testkit._
import org.scalatest._
import nfn.service.NFNServiceLibrary
import lambdacalculus.LambdaCalculus
import lambdacalculus.parser.ast._
import nfn.NFNServer._
import ccn.packet._
import nfn.service.impl.{WordCountService, AddService}


class NFNMasterSpec(_system: ActorSystem) extends TestKit(_system) with ImplicitSender
with WordSpecLike with Matchers with BeforeAndAfterEach with BeforeAndAfterAll with SequentialNestedSuiteExecution {

  println("INIT")

  val nodeConfig = NodeConfig("localhost",10000, 10001, CCNName("testnode"))
  val nfnMasterNetworkRef: TestActorRef[NFNServer] = TestActorRef(CCNServerFactory.networkProps(nodeConfig))
  val nfnMasterNetworkInstance = nfnMasterNetworkRef.underlyingActor

  val name = CCNName("test", "data")

  val ts = System.currentTimeMillis.toString
  val doc1Name = Seq("test", ts, "doc", "1")
  val doc2Name = Seq("test", ts, "doc", "2")

  val doc1Content = Content("one two".getBytes, doc1Name:_*)
  val doc2Content = Content("one two three".getBytes, doc2Name:_*)

  val data = "test".getBytes
  val interest = Interest(name)
  val content = Content(name, data)

  val lc = LambdaCalculus()

  def this() = this(ActorSystem("NFNMasterSpec"))

  override def beforeAll() {
    def initCaches(ref: ActorRef) = {
      ref ! NFNApi.AddToCCNCache(content)
      ref ! NFNApi.AddToCCNCache(doc1Content)
      ref ! NFNApi.AddToCCNCache(doc2Content)
      NFNServiceLibrary.nfnPublish(ref)
    }

    initCaches(nfnMasterNetworkRef)
    Thread.sleep(100)
  }

//  testNFNMaster(nfnMasterLocalRef, "NFNMasterLocal")
  testNFNMaster(nfnMasterNetworkRef, "NFNMasterNetwork", nfnNetwork = true)

  override def afterAll() {
    TestKit.shutdownActorSystem(system)
  }


  def testNFNMaster(nfnMasterRef: ActorRef, nfnMasterName: String, nfnNetwork: Boolean = false) = {

    def testComputeRequest(req: String, result: String) = {
      val parsedExpr = lc.parse(req).get
      val parsedReq = if(nfnNetwork) LambdaNFNPrinter(parsedExpr) else LambdaPrettyPrinter(parsedExpr)

      val temporaryPaddedResult = if(nfnNetwork) {"RST|" + result } else { result }

      s"compute result '$temporaryPaddedResult' for ${if(nfnNetwork) "localAbstractMachine" else "nfn"} request '$parsedReq'" in {

        val computeReqName = Seq(parsedReq, "NFN")
        nfnMasterRef ! NFNApi.CCNSendReceive(Interest(computeReqName:_*))
        val actualContent = expectMsgType[Content]
        actualContent.name shouldBe computeReqName
        actualContent.data shouldBe temporaryPaddedResult.getBytes
      }
    }

    s"An $nfnMasterName actor" should {
      "send interest and receive corresponding data" in {
        nfnMasterRef ! NFNApi.CCNSendReceive(interest)
        val actualContent = expectMsgType[Content]
        actualContent.data shouldBe data
      }

      "add content to cache" in {
        val contentName = Seq("name", "addtocache")
        val contentData = "added to cache!".getBytes
        val cacheContent = Content(contentData, contentName:_*)
        nfnMasterRef ! NFNApi.AddToCCNCache(cacheContent)
        // TODO maybe make this nicer?
        Thread.sleep(200)
        nfnMasterRef ! NFNApi.CCNSendReceive(Interest(contentName:_*))
        val actualContent = expectMsgType[Content]
        actualContent.data shouldBe contentData
      }
      testComputeRequest("1 ADD 2", "3")
      testComputeRequest(s"call 3 ${AddService().ccnName.toString} 12 30", "42")
      testComputeRequest(s"1 ADD call 3 ${AddService().ccnName.toString} 11 29", "41")
      testComputeRequest(s"call 3 ${WordCountService().toString} ${doc1Name.mkString("/", "/", "")} ${doc2Name.mkString("/", "/", "")}", "5")

//      testComputeRequest(s"call 1 ${WordCountService().nfnName.toString}", "0")
    }
  }
}

