package monitor

import monitor.Monitor._
import ccn.ccnlite.CCNLite
import network.NFNCommunication
import ccnliteinterface.CCNLiteInterface
import ccn.packet.Interest
import scala.collection.immutable.Iterable
import net.liftweb.json._
import net.liftweb.json.Serialization.{read, write}
import net.liftweb.json.JsonDSL._
import scala.Some
import scala.util.parsing.json
import scala.io.Source
import myutil.IOHelper
import java.io.File
import com.typesafe.scalalogging.slf4j.Logging

case class Packets(packets: List[TransmittedPacket])
case class TransmittedPacket(`type`: String, from: NodeLog, to: NodeLog, timeMillis: Long, transmissionTime: Long, packet: CCNPacketLog)
/**
 * Created by basil on 23/04/14.
 */
case class OmnetIntegration(nodes: Set[NodeLog],
                            edges: Set[Pair[NodeLog, NodeLog]],
                            loggedPackets: Set[PacketLog],
                            simStart: Long) extends Logging {


  def packets = {

    def loggedPacketToType(lp: PacketLog): String = if (lp.packet.isInstanceOf[InterestLog]) "interest" else "content"

    def actuallyTransmittedPackets: List[TransmittedPacket] = {
      println(s"LOGGED PACKETS: $loggedPackets")
      loggedPackets.map({
        lp1 =>
          val maybeReceiveLog: Option[PacketLog] = loggedPackets.find {
            lp2 =>
              lp1.from == lp2.from &&
                lp1.to == lp2.to &&
                lp1.isSent != lp2.isSent &&
                lp1.packet == lp2.packet &&
                lp1.isSent
          }
          maybeReceiveLog map {
            lp2 =>
              val transmissionTime = lp2.timestamp - lp1.timestamp
              val `type` = loggedPacketToType(lp1)
              TransmittedPacket(`type`, lp1.from, lp1.to, lp1.timestamp - simStart, transmissionTime, lp1.packet)
          }
      }).collect({
        case Some(transmittedPacket: TransmittedPacket) => transmittedPacket
      }).toList
    }

    def transmittedPackets: List[TransmittedPacket] = {
      loggedPackets.toList.sortBy(_.timestamp).map({ lp =>
        val `type` = loggedPacketToType(lp)
        TransmittedPacket(`type`, lp.from, lp.to, lp.timestamp - simStart, 2, lp.packet)
      })
    }

    Packets(transmittedPackets)
  }

  def apply() = {

    import IOHelper.printToFile
    val nedContent = createNed()
    val nedFilename = "./omnetintegration/NFNNetwork.ned"

    printToFile(new File(nedFilename), nedContent)
    logger.info(s"Wrote .ned file to $nedFilename")

    val transmittedPacketsJson = createTransmissionJson
    val transmittedPacketsJsonFilename = "./omnetintegration/transmittedPackets.json"

    printToFile(new File(transmittedPacketsJsonFilename), transmittedPacketsJson)
    logger.info(s"Wrote transmittedPackets to .json file $transmittedPacketsJsonFilename")

  }

  def createTransmissionJson:String = {
    implicit val formats = Serialization.formats(NoTypeHints)
    write(packets)
  }


  def createNed(): String = {
    val submodules: Seq[String] = nodes.map ({ node =>
        val name = node.prefix
        val typ = node.typ
        s"$name: $typ;"
    }).toSeq

    val connections: Seq[String] = edges.groupBy{ edge =>
      edge._1.prefix
    }.flatMap({ case (prefix, edgesFromNode: Set[(NodeLog, NodeLog)]) =>
      edgesFromNode map { edge =>
        val from = edge._1.prefix
        val to = edge._2.prefix
        val fromGate = to
        val toGate = from
        s"$from.out++ --> { delay = 1ms; } --> $to.in++;"
      }
    }).toSeq


    s"""
      |simple Node
      |{
      |    parameters:
      |        @display("i=block/routing");
      |    gates:
      |        input in[];
      |        output out[];
      |
      |}
      |
      |simple NFNNode extends Node
      |{
      |    parameters:
      |        @display("i=,cyan");
      |}
      |
      |simple ComputeNode extends Node
      |{
      |    parameters:
      |        @display("i=,gold");
      |}
      |
      |network NFNNetwork
      |{
      |    submodules:
      |        ${submodules.mkString("\n" + " " * 8)}
      |    connections:
      |        ${connections.mkString("\n" + " " * 8)}
      |}
      |
    """.stripMargin
  }
}
