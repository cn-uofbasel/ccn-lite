package monitor

import nfn.NodeConfig
import monitor.MonitorActor.{ContentLog, InterestLog, PacketLog, NodeLog}
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
      NodeLog("localhost", 1, Some("docRepo1"), Some("ComputeNode")),
      NodeLog("localhost", 2, Some("docRepo2"), Some("ComputeNode")),
      NodeLog("localhost", 3, Some("docRepo3"), Some("NFNNode")),
      NodeLog("localhost", 4, Some("docRepo4"), Some("NFNNode"))
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
    PacketLog(nodes(0), nodes(1), isSent = true, InterestLog("/interest/name")),
    PacketLog(nodes(0), nodes(1), isSent = false, InterestLog("/interest/name")),
    PacketLog(nodes(1), nodes(0), isSent = true, ContentLog("/interest/name", "testcontent")),
    PacketLog(nodes(1), nodes(0), isSent = false, ContentLog("/interest/name", "testcontent"))
  )

  OmnetIntegration(nodes.toSet, edges, packets, starttime)()
}
