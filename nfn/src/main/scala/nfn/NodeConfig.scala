package nfn

import monitor.Monitor.NodeLog
import ccn.packet.CCNName



trait NodeConfig {
  def host: String
  def port: Int
  def prefix: CCNName
  def toNodeLog: NodeLog
}

case class CombinedNodeConfig(maybeNFNNodeConfig: Option[NFNNodeConfig], maybeComputeNodeConfig: Option[ComputeNodeConfig])

/**
 * Created by basil on 09/05/14.
 */
case class NFNNodeConfig(host: String, port: Int, prefix: CCNName, isCCNOnly: Boolean = false) extends NodeConfig {
  def toNodeLog: NodeLog = NodeLog(host, port, Some("NFNNode"), Some(prefix.toString))
}


case class ComputeNodeConfig(host: String, port: Int, prefix: CCNName, withLocalAM: Boolean = false) extends NodeConfig {
  def toNodeLog: NodeLog = NodeLog(host, port, Some("ComputeNode"), Some(prefix + "compute"))
}
