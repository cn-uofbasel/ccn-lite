package nfn

import java.util.concurrent.TimeUnit

import com.typesafe.config.{ConfigFactory, Config}
import monitor.Monitor.NodeLog
import ccn.packet.CCNName

import scala.concurrent.duration.Duration

object StaticConfig {

  private var maybeConfig: Option[Config] = None

  def config: Config = maybeConfig match {
    case Some(config) => config
    case None => {
      val conf = ConfigFactory.load()
      maybeConfig = Some(conf)
      conf
    }
  }

  def isNackEnabled =  config.getBoolean("nfn-scala.usenacks")

  def isThunkEnabled = config.getBoolean("nfn-scala.usethunks")

  def defaultTimeoutDuration = Duration(config.getInt("nfn-scala.defaulttimeoutmillis"), TimeUnit.MILLISECONDS)

  def debugLevel = config.getString("nfn-scala.debuglevel")
}

trait NodeConfig {
  def host: String
  def port: Int
  def prefix: CCNName

  import StaticConfig._
  def toNodeLog: NodeLog


}

case class CombinedNodeConfig(maybeNFNNodeConfig: Option[RouterConfig], maybeComputeNodeConfig: Option[ComputeNodeConfig])

case class RouterConfig(host: String, port: Int, prefix: CCNName, isCCNOnly: Boolean = false)(implicit conf: Config) extends NodeConfig {
  def toNodeLog: NodeLog = NodeLog(host, port, Some(if(isCCNOnly) "CCNNode" else "NFNNode"), Some(prefix.toString))
  def config = conf
}


case class ComputeNodeConfig(host: String, port: Int, prefix: CCNName, withLocalAM: Boolean = false)(implicit conf: Config) extends NodeConfig {
  def toNodeLog: NodeLog = NodeLog(host, port, Some("ComputeNode"), Some(prefix + "compute"))
  def config = conf
}
