package monitor

import akka.actor._
import akka.testkit._
import org.scalatest._
import net.liftweb.json._
import net.liftweb.json.Serialization.{read, write}
import monitor.Monitor._


class MonitorSpec(_system: ActorSystem) extends TestKit(_system)
                                             with ImplicitSender
                                             with WordSpecLike
                                             with Matchers
                                             with BeforeAndAfterEach
                                             with BeforeAndAfterAll
                                             with SequentialNestedSuiteExecution {

  val monitorRef: TestActorRef[Monitor] = TestActorRef(Props(classOf[Monitor]))
  val monitorInstance = monitorRef.underlyingActor

  "A packet log" should {
    "be writable to json and back to an instance" in {
      val node1 = NodeLog("testhost1", 1, Some("testtype"), Some("testprefix"))
      val node2 = NodeLog("testhost1", 1, Some("testtype"), Some("testprefix"))

      val testPacket = InterestInfoLog("interest", "/test/Interest")

      val pl = new PacketLog(node1, node2, true, testPacket)
      import monitorInstance.formats
      val json: String = write(pl)

      val parsedJson = monitorInstance.parseJsonData(json.getBytes)
      parsedJson shouldNot be (None)
    }
  }
}


