package monitor

import monitor.Monitor._
import net.liftweb.json._
import net.liftweb.json.Serialization.write
import myutil.IOHelper
import java.io.File
import com.typesafe.scalalogging.slf4j.Logging
import scala.Some

case class Packets(packets: List[TransmittedPacket])
case class TransmittedPacket(`type`: String, from: NodeLog, to: NodeLog, timeMillis: Long, transmissionTime: Long, packet: PacketInfoLog)

case class OmnetIntegration(nodes: Set[NodeLog],
                            edges: Set[Pair[NodeLog, NodeLog]],
                            loggedPackets: Set[PacketLog],
                            simStart: Long) extends Logging {


  def loggedPacketToType(lp: PacketInfoLog): String = lp match {
    case i: InterestInfoLog => "interest"
    case c: ContentInfoLog => "content"
    case _ => "unkown"
  }
  def packets: Packets = {

    def transmittedPackets: List[TransmittedPacket] = {
      loggedPackets.toList.sortBy(_.timestamp).map({ lp =>
        val `type` = loggedPacketToType(lp.packet)
        lp.from match {
          case Some(from) =>
            if(lp.isSent) {
              Some(TransmittedPacket(`type`, from, lp.to, lp.timestamp - simStart, 2, lp.packet))
            } else {
              None
            }
          case None => {
              logger.warn(s"The packet log $lp does not have a from address, discarding it")
              None
          }
        }
      }).collect( {case Some(tp) => tp })
    }

    Packets(transmittedPackets)
  }


  def apply() = {

//    logger.debug(s"NODES: $nodes")
//    logger.debug(s"EDGES: $edges")
//    logger.debug(s"PACKETS:\n${loggedPackets.mkString("\n")}")

    val sortedPackets: List[PacketLog] = loggedPackets.toList.sortBy(_.timestamp)

//    logger.debug(s"SORTED PACKETS")
//    sortedPackets foreach { lp =>
//      logger.debug(s"${lp.from.get.host}:${lp.from.get.port} -> ${lp.to.host}: ${lp.to.port}: ${lp.packet.name} (${lp.timestamp})")
//    }
//    logger.debug(s"SORTED PACKETS:\n${sortedPackets.mkString("\n")}")

    import IOHelper.printToFile
    val nedContent = createNed()

    val nedFilename = "./omnetreplay/NFNNetwork.ned"

    printToFile(new File(nedFilename), nedContent)
    logger.info(s"Wrote .ned file to $nedFilename")

    val transmittedPacketsJson = createTransmissionJson
    val transmittedPacketsJsonFilename = "./omnetreplay/transmittedPackets.json"

    printToFile(new File(transmittedPacketsJsonFilename), transmittedPacketsJson)
    logger.info(s"Wrote transmittedPackets to .json file $transmittedPacketsJsonFilename")

  }

  def createTransmissionJson:String = {
    implicit val formats = Serialization.formats(NoTypeHints)
    import net.liftweb.json.JsonDSL._

    def jsonPacket(p: PacketInfoLog): JValue =
      p match {
        case i: InterestInfoLog =>
        ("name" -> i.name) ~ ("type" -> "interest")
        case c: ContentInfoLog =>
        ("name" -> c.name) ~ ("data" -> c.data) ~ ("type" -> "content")
        case _ => throw new Exception("asdf")
      }

    val json: JsonAST.JValue =
    ("packets" ->
      packets.packets.map { p =>
        ("from" ->
          ("host" -> p.from.host) ~
          ("port" -> p.from.port) ~
          ("prefix" -> p.from.prefix.map(_.replace("/", ""))) ~
          ("type" -> p.from.`type`)) ~
        ("to" ->
          ("host" -> p.to.host) ~
          ("port" -> p.to.port) ~
          ("prefix" -> p.to.prefix.map(_.replace("/", ""))) ~
          ("type" -> p.to.`type`)) ~
        ("packet" -> jsonPacket(p.packet)) ~
        ("timeMillis" -> p.timeMillis) ~
        ("transmissionTime" -> p.transmissionTime) ~
        ("type" -> loggedPacketToType(p.packet))
      } )
    pretty(render(json))

  }


  def createNed(): String = {
    def prefixOrDefaultName(nl: NodeLog) = nl.prefix.getOrElse(s"${nl.host}${nl.port}").replace("/", "")

    def typeOrDefaultType(nl: NodeLog): String = nl.`type`.getOrElse("DefaultNode")

    val submodules: Seq[String] = nodes.map ({ node =>
        val name = prefixOrDefaultName(node)
        val `type` = typeOrDefaultType(node)
        s"$name: ${`type`};"
    }).toSeq

    val connections: Seq[String] = edges.groupBy{ edge =>
      prefixOrDefaultName(edge._1)
    }.flatMap({ case (_, edgesFromNode: Set[(NodeLog, NodeLog)]) =>
      edgesFromNode map { (edge: (NodeLog, NodeLog)) =>

        val from = prefixOrDefaultName(edge._1)
        val to = prefixOrDefaultName(edge._2)
        s"$from.out++ --> $to.in++;"
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
      |simple CCNNode extends Node
      |{
      |    parameters:
      |       @display("i=,green");
      |}
      |
      |simple ComputeNode extends Node
      |{
      |    parameters:
      |        @display("i=,gold");
      |}
      |
      |
      |simple DefaultNode extends Node
      |{
      |    parameters:
      |        @display("i=,gray");
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
