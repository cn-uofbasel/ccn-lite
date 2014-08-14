package monitor

import akka.actor.{Actor, ActorSystem, Props}
import ccn.packet.CCNPacket
import config.AkkaConfig
import java.net.InetSocketAddress
import network.UDPConnection
import akka.event.Logging
import net.liftweb.json._
import monitor.Monitor._
import scala.util.Try
import scala.util.Failure
import scala.Some
import scala.util.Success
import net.liftweb.json.ShortTypeHints


object Monitor {

  val startTime = System.nanoTime
  val host = "localhost"
  val port = 10666

  private val system = ActorSystem(s"Monitor", AkkaConfig.config("WARNING"))
  val monitor = system.actorOf(Props(classOf[Monitor]))

  case class Visualize()

  sealed trait MonitorLogEntry {
    val timestamp: Long = System.nanoTime
  }

  case class NodeLog(host: String, port: Int, `type`: Option[String], prefix: Option[String]) extends MonitorLogEntry
  case class ConnectLogEntry(connectLog: ConnectLog)
  case class ConnectLog(from: NodeLog, to: NodeLog) extends MonitorLogEntry

  sealed trait PacketInfoLog {
    def `type`: String
    def name: String
  }
  case class ContentInfoLog(`type`: String = "content", name: String, data: String) extends PacketInfoLog
  case class InterestInfoLog(`type`: String = "interest", name: String) extends PacketInfoLog

  case class PacketLogWithoutConfigs(fromHost: String, fromPort: Int, toHost: String, toPort: Int, isSent: Boolean, packet: PacketInfoLog ) extends MonitorLogEntry


  case class PacketLog(from: Option[NodeLog], to: NodeLog, isSent: Boolean, override val timestamp: Long, packet: PacketInfoLog) extends MonitorLogEntry {
    def this(from: NodeLog, to: NodeLog, isSent: Boolean, packet: PacketInfoLog) =
      this(Some(from), to, isSent, System.nanoTime ,packet)
    def this(from: Option[NodeLog], to: NodeLog, isSent: Boolean, packet: PacketInfoLog) =
      this(from, to, isSent, System.nanoTime ,packet)
//    def this(from: Option[NodeLog], to: NodeLog, isSent: Boolean, timestamp: Long, packet: CCNPacketLog) =
//      this(from, to, isSent, timestamp,packet)
  }

  case class AddToCacheLogEntry(addToCacheLog: AddToCacheLog)
  case class AddToCacheLog(node: NodeLog, ccnb: String) extends MonitorLogEntry

  case class Send(packet: CCNPacket)
}


case class Monitor() extends Actor {

  case class HostAndPort(host: String, port: Int)

  val logger = Logging(context.system, this)

  val nfnSocket = context.actorOf(
    Props(classOf[UDPConnection],
      new InetSocketAddress(Monitor.host, Monitor.port),
      None),
    name = s"udpsocket-${Monitor.port}"
  )

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

  override def preStart() {
    nfnSocket ! UDPConnection.Handler(self)
    logger.debug("started Monitor")
  }

  def findSameNode(nodeLog: NodeLog): Option[NodeLog] = findNode(nodeLog.host, nodeLog.port)

  def findNode(host: String, port: Int): Option[NodeLog] = {
    def sameNodeWithName: NodeLog => Boolean = { nl => sameNode(nl) && nl.`type` != None && nl.prefix != None }
    def sameNode: NodeLog => Boolean = { nl => nl.host == host && nl.port == port }

    nodes find sameNodeWithName orElse { nodes find sameNode }
  }

  def handlePacketLog(log: PacketLog, fromHostAndPort: Option[HostAndPort] = None): Unit = {
    // If no host and port was passed to the function, try to take it form the log (might still be None)
    val maybeFrom: Option[NodeLog] =
      fromHostAndPort.fold(log.from) { hostAndPort =>
        findNode(hostAndPort.host, hostAndPort.port)
      } flatMap { findSameNode }

    val maybeTo: Option[NodeLog] = findNode(log.to.host, log.to.port) flatMap findSameNode

    maybeFrom -> maybeTo match {
      case (Some(from), Some(to)) => {
        logger.debug(s"Logging packet log $log")
        loggedPackets += log
      }
      case (from, to) => logger.error(s"Could not find sender ($from) or receiver ($to) for log: $log and from $fromHostAndPort\n[$nodes]")
    }
  }
  def handleAddToCacheLog(log: AddToCacheLog): Unit =  addedToCache += log

  implicit val formats =
    DefaultFormats.withHints(
      ShortTypeHints(List(classOf[InterestInfoLog], classOf[ContentInfoLog])) )

  def handleConnectLogJson(json: JsonAST.JValue): Unit = {
    json.extractOpt[ConnectLog] match {
      case Some(log) => handleConnectLog(log)
      case None => logger.error(s"Error when extracting ConnectLogRequest from json '$json'")
    }
  }

  def handlePacketLogJsonFrom(json: JsonAST.JValue, fromHostAndPort: HostAndPort): Unit = {

    try {
      val maybeParsedFrom = (json \\ "from").extractOpt[NodeLog]
      val parsedTo = (json \\ "to").extract[NodeLog]
      val isSent = (json \\ "isSent").extract[Boolean]
      val timestamp = (json \\ "timestamp").extract[Long]
      val parsedPacket: JsonAST.JValue = json \\ "packet"

//      JObject(List(JField(type,JString(interest)), JField(name,JString(/docRepo1/docs/doc0/(@x call 2 /nfn_service_impl_WordCountService x )/NFN))))

      val name = (parsedPacket \\ "name").extract[String]
      val packet: PacketInfoLog =
      (parsedPacket \\ "type").extract[String] match {
        case t @ "interest" => {
          InterestInfoLog(t, name)
        }
        case t @ "content" =>
          val data = (parsedPacket \\ "data").extract[String]
          ContentInfoLog(t, name, data)
        case _ =>
          logger.error("error when parsing packet")
          throw new Exception("error when parsing packet")
      }

      val from = maybeParsedFrom.orElse(findNode(fromHostAndPort.host, fromHostAndPort.port))
      val to = findSameNode(parsedTo).getOrElse {
        logger.warning(s"packet log $parsedTo with a to address which is not stored in nodes: $nodes")
        parsedTo
      }

      val pl = PacketLog(from, to, isSent, timestamp, packet)

      if(from == None) logger.warning(s"no from node found: $pl")

      handlePacketLog(pl, Some(fromHostAndPort))
    } catch {
      case e: Exception => logger.error(e, s"error when handling json for packet log $json")
    }

  }

  def handleAddToCacheLogJson(json: JsonAST.JValue): Unit = {
    json.extractOpt[AddToCacheLog] match {
      case Some(log) => handleAddToCacheLog(log)
      case None => logger.error(s"Error when extracting AddToCache from json '$json'")
    }
  }


  def parseJsonData(data: Array[Byte]): Option[JValue] = {
    val json = new String(data)
    logger.debug(s"Received json data $json")
    val triedParsedJson: Try[JValue] = Try(parse(json))

    triedParsedJson match {
      case Success(parsedJson) =>
        logger.debug(s"Parsed json: $parsedJson")
        Some(parsedJson )
      case Failure(e) =>
        logger.error(e, "Could not parse json")
        None
    }
  }

  def handleJsonFrom(hostAndPort: HostAndPort): JValue => Unit = {
    case JObject(actions) => actions foreach {
      case JField("connectLog", connectJson) => handleConnectLogJson(connectJson)
      case JField("packetLog", packetJson) => {
        logger.debug("packet log json parsed")
        handlePacketLogJsonFrom(packetJson, hostAndPort)
      }
      case JField("addToCacheLog", addToCacheJson) => handleAddToCacheLogJson(addToCacheJson)
      case json @ _ => logger.error(s"Parsed json packet with unkown action $json")
    }
    case parsedJson @ _ => logger.error(s"parsed json must be a JObject (list of actions to log), but it was $parsedJson")
  }
  def parseAndHandleJsonDataFrom(data: Array[Byte], hostAndPort: HostAndPort): Unit = {
    parseJsonData(data) map { handleJsonFrom(hostAndPort) }
  }

  def ipHost(host: String): String = if(host == "localhost") "127.0.0.1" else host

  override def receive: Receive = {
    case cl: ConnectLogEntry => handleConnectLog(cl.connectLog)
//    case pl: PacketLogEntry => handlePacketLog(pl.packetLog)
    case atc: AddToCacheLogEntry => handleAddToCacheLog(atc.addToCacheLog)

    case cl: ConnectLog => handleConnectLog(cl)
    case pl: PacketLog => handlePacketLog(pl)
    case atc: AddToCacheLog => handleAddToCacheLog(atc)

    case plWithoutConfig: PacketLogWithoutConfigs => {
      val maybeFrom = findNode(plWithoutConfig.fromHost, plWithoutConfig.fromPort)
      val maybeTo = findNode(plWithoutConfig.toHost, plWithoutConfig.toPort)

      maybeTo match {
        case Some(to) => {
          val pl = PacketLog(maybeFrom, to,plWithoutConfig.isSent, plWithoutConfig.timestamp, plWithoutConfig.packet)
          self.tell(pl, sender)
        }
        case None => logger.error(s"Receiver of packet for a packetlog without config does not exist: $plWithoutConfig")
      }
    }

    case Monitor.Visualize() => {
      OmnetIntegration(nodes, edges, loggedPackets, startTime)()
    }
    case UDPConnection.Received(data, sendingRemote: InetSocketAddress) => {
      val host = ipHost(sendingRemote.getHostName)
      val port = sendingRemote.getPort
      parseAndHandleJsonDataFrom(data, HostAndPort(host, port))
    }
  }
}
