package nfn

import monitor.Monitor.NodeLog
import ccn.packet.CCNName

/**
 * Created by basil on 09/05/14.
 */
case class NodeConfig(host: String, port: Int, computePort: Int, prefix: CCNName) {
  def toNFNNodeLog: NodeLog = NodeLog(host, port, Some("NFNNode"), Some(prefix.toString))
  def toComputeNodeLog: NodeLog = NodeLog(host, computePort, Some("ComputeNode"), Some(prefix + "compute"))
}
