package monitor

import nfn.NodeConfig
import monitor.Monitor.{ContentLog, InterestLog, PacketLog, NodeLog}
import ccn.packet.Interest
import ccn.ccnlite.CCNLite
import ccnliteinterface.CCNLiteInterface
import network.NFNCommunication

/**
 * Created by basil on 17/04/14.
 */
object TestApp extends App {

  val starttime = System.currentTimeMillis()

  Thread.sleep(5)
  val nodes =
    Seq(
      NodeLog("127.0.0.1", 1, Some("docRepo1"), Some("ComputeNode")),
      NodeLog("127.0.0.1", 2, Some("docRepo2"), Some("ComputeNode")),
      NodeLog("127.0.0.1", 3, Some("docRepo3"), Some("NFNNode")),
      NodeLog("127.0.0.1", 4, Some("docRepo4"), Some("NFNNode"))
    )

  val edges = Set(
    Pair(nodes(0), nodes(1)),
    Pair(nodes(1), nodes(0)),
    Pair(nodes(1), nodes(2)),
    Pair(nodes(2), nodes(3)),
    Pair(nodes(3), nodes(0)),
    Pair(nodes(0), nodes(2)),
    Pair(nodes(2), nodes(1))
  )

  val interest = Interest(Seq("name"))

  val packets = Set(
    new PacketLog(nodes(0), nodes(1), isSent = true, InterestLog("interst", "/interest/name")),
    new PacketLog(nodes(0), nodes(1), isSent = false, InterestLog("interst", "/interest/name")),
    new PacketLog(nodes(1), nodes(0), isSent = true, ContentLog("content", "/interest/name", "testcontent")),
    new PacketLog(nodes(1), nodes(0), isSent = false, ContentLog("content", "/interest/name", "testcontent"))
  )

  OmnetIntegration(nodes.toSet, edges, packets, starttime)()
}
