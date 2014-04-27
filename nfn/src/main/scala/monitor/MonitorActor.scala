package monitor

import akka.actor.{Actor, ActorSystem, Props}
import ccn.packet.CCNPacket
import config.AkkaConfig
import java.net.InetSocketAddress
import network.UDPConnection
import akka.util.ByteString
import akka.event.Logging
import net.liftweb.json._
import monitor.MonitorActor._


object MonitorActor {

  val host = "localhost"
  val port = 10666

  private val system = ActorSystem(s"Monitor", AkkaConfig())
  val monitor = system.actorOf(Props(classOf[Monitor]))

  case class Visualize()

  sealed trait MonitorLogEntry {
    val timestamp = System.currentTimeMillis
  }

  case class NodeLog(host: String, port: Int, `type`: Option[String], prefix: Option[String]) extends MonitorLogEntry
  case class ConnectLogEntry(connectLog: ConnectLog)
  case class ConnectLog(from: NodeLog, to: NodeLog) extends MonitorLogEntry

  trait CCNPacketLog {
    def `type`: String
  }
  case class ContentLog(name: String, data: String) extends CCNPacketLog {
    val `type`: String = "content"
  }
  case class InterestLog(name: String) extends CCNPacketLog {
    val `type`: String = "interest"
  }

  /**
   * {
   *   "packetLog": {
   *     "from": {
   *       "host": "127.0.0.1",
   *       "port": 10001,
   *       "type" : "NFNNode",  // optional
   *       "prefix": "docrepo1" // optional
   *     },
   *     "to": {
   *       "host": "127.0.0.1",
   *       "port": 10002
   *     },
   *     "isSent": true,
   *     "interest": {
   *       "name" : "/ccn/name",
   *       "data" : "TWFuIGlzIGZmF0aWdhYmxlIGdlbmVyYXRpb24" // base 64
   *     }
   *   }
   * }
   */
  case class PacketLogEntry(packetLog: PacketLog)
  case class PacketLog(from: NodeLog, to: NodeLog, isSent: Boolean, packet: CCNPacketLog) extends MonitorLogEntry

  case class AddToCacheLogEntry(addToCacheLog: AddToCacheLog)
  case class AddToCacheLog(node: NodeLog, ccnb: String) extends MonitorLogEntry

  case class Send(packet: CCNPacket)
}


case class Monitor() extends Actor {


  val logger = Logging(context.system, this)

  val system = ActorSystem(s"Monitor", AkkaConfig())
  val nfnSocket = system.actorOf(
    Props(classOf[UDPConnection],
      new InetSocketAddress(MonitorActor.host, MonitorActor.port),
      None),
    name = s"udpsocket-${MonitorActor.port}"
  )

  val startTime = System.currentTimeMillis()




  var nodes = Set[NodeLog]()

  var edges = Set[Pair[NodeLog, NodeLog]]()

  var loggedPackets = Set[PacketLog]()

  var addedToCache = Set[AddToCacheLog]()

  def handleConnectLog(log: ConnectLog): Unit = {
    import log._
    if(!edges.contains(Pair(from, to))) {
      nodes += to
      nodes += from
      edges += Pair(from, to)
    }
  }

  def handlePacketLog(log: PacketLog): Unit = {
    def findNode(nodeLog: NodeLog): Option[NodeLog] = {
      import nodeLog._
      nodes.find {
        nl => nl.host == host && nl.port == port
      }
    }

    import log._
    findNode(from) -> findNode(to) match {
      case (Some(sndr), Some(rcvr)) => loggedPackets += log
      case (sndr, rcvr) => logger.error(s"Could not find sender or receiver: ($sndr, $rcvr)")
    }
  }
  def handleAddToCacheLog(log: AddToCacheLog): Unit =  addedToCache += log

  implicit val formats = DefaultFormats

  def handleConnectLogJson(json: JsonAST.JValue): Unit = {
    json.extractOpt[ConnectLog] match {
      case Some(log) => handleConnectLog(log)
      case None => logger.error(s"Error when extracting ConnectLogRequest from json '$json'")
    }
  }

  def handlePacketLogJson(json: JsonAST.JValue): Unit = {
    json.extractOpt[PacketLog] match {
      case Some(log) => handlePacketLog(log)
      case None => logger.error(s"Error when extracting PacketLog from json '$json'")
    }
  }

  def handleAddToCacheLogJson(json: JsonAST.JValue): Unit = {
    json.extractOpt[AddToCacheLog] match {
      case Some(log) => handleAddToCacheLog(log)
      case None => logger.error(s"Error when extracting AddToCache from json '$json'")
    }
  }


  def parseAndHandleJsonData(data: ByteString): Unit = {
    val json = data.decodeString("UTF-8")
    parseOpt(json) map {
      case JField("connectLog", connectJson) => handleConnectLogJson(connectJson)
      case JField("packetLog", packetJson) => handlePacketLogJson(packetJson)
      case JField("addToCacheLog", addToCacheJson) => handleAddToCacheLogJson(addToCacheJson)
      case _ => logger.error(s"Could not parse json string '$json'")
    }
  }

  override def receive: Receive = {
    case cl: ConnectLogEntry => handleConnectLog(cl.connectLog)
    case pl: PacketLogEntry => handlePacketLog(pl.packetLog)
    case atc: AddToCacheLogEntry => handleAddToCacheLog(atc.addToCacheLog)
    case cl: ConnectLog => handleConnectLog(cl)
    case pl: PacketLog => handlePacketLog(pl)
    case atc: AddToCacheLog => handleAddToCacheLog(atc)

    case MonitorActor.Visualize() => {
      OmnetIntegration(nodes, edges, loggedPackets, startTime)()
    }
    case data: ByteString => {
      parseAndHandleJsonData(data)
    }
  }
}

