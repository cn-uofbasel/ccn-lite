package node

import java.util.concurrent.TimeUnit

import com.typesafe.config.Config
import nfn._
import scala.concurrent.duration.FiniteDuration
import scala.concurrent.ExecutionContext.Implicits.global
import akka.util.Timeout
import akka.actor.{ActorRef, ActorSystem}
import akka.pattern._
import config.AkkaConfig
import ccn.packet._
import scala.concurrent.Future
import scala.concurrent.duration._
import nfn.service.{NFNService, NFNServiceLibrary}
import scala.collection.immutable.{Iterable, IndexedSeq}
import ccn.CCNLiteProcess
import monitor.Monitor
import ccn.CCNLiteProcess

object StandardNodeFactory {
  def forId(id: Int, isCCNOnly: Boolean = false)(implicit config: Config): LocalNode = {
    val nodePrefix = CCNName("node", s"node$id")
    LocalNode(
      RouterConfig("127.0.0.1", 10000 + id * 10, nodePrefix, isCCNOnly = isCCNOnly),
      Some(ComputeNodeConfig("127.0.0.1", 10000 + id * 10 + 1, nodePrefix, withLocalAM = false))
    )
  }
}

object LocalNode {
  /**
   * Connects the given sequence to a grid.
   * The size of the sequence does not have to be a power of a natural number.
   * Example:
   * o-o-o    o-o-o
   * | | |    | | |
   * o-o-o or o-o-o
   * | | |    | |
   * o-o-o    o-o
   * @param nodes
   */
  def connectGrid(nodes: Seq[LocalNode]): Unit = {
    if(nodes.size <= 1) return

    import Math._
    val N = nodes.size
    val roundedN = sqrt(N).toInt
    val n = roundedN + (if(N / roundedN > 0) 1 else 0)
    println(s"n: $n N: $N")

    val horizontalLineNodes = nodes.grouped(n)
    horizontalLineNodes foreach { connectLine }

    // 0 1 2    0 3 6
    // 3 4 5 -> 1 4 7
    // 6 7 8    2 5 8
    val reshuffledNodes = 0 until N map { i =>
      val index = (i/n + n * (i % n)) % N
      println(index)
      nodes(index)
    }

    val verticalLineNodes = reshuffledNodes.grouped(n)
    verticalLineNodes foreach { connectLine }
  }

  /**
   * Connects head with every node in tail, this results in a star shape.
   * Example:
   *   o
   *   |
   * o-o-o
   * @param nodes
   */
  def connectStar(nodes: Seq[LocalNode]): Unit = {
    if(nodes.size <= 1) return

    nodes.tail foreach { _ <~> nodes.head }
  }

  /**
   * Connects every node with every other node (fully connected).
   * o - o
   * | X |
   * o - o
   * @param unconnectedNodes
   */
  def connectAll(unconnectedNodes: Seq[LocalNode]): Unit = {
    if(unconnectedNodes.size <= 1 ) return

    val head = unconnectedNodes.head
    val tail = unconnectedNodes.tail

    tail foreach { _ <~> head }
    connectAll(tail)
  }

  /**
   * Connects the nodes along the given sequence.
   * Example:
   * o-o-o-o
   * @param nodes
   * @return
   */
  def connectLine(nodes: Seq[LocalNode]): Unit = {
    if(nodes.size <= 1) return

    nodes.tail.foldLeft(nodes.head) {
      (left, right) => left <~> right; right
    }
  }
}

/**
 * Created by basil on 10/04/14.
 */
case class LocalNode(routerConfig: RouterConfig, maybeComputeNodeConfig: Option[ComputeNodeConfig]){

  implicit val timeout = Timeout(StaticConfig.defaultTimeoutDuration)

  private var isRunning = true

  val prefix = routerConfig.prefix

  /**
   * This flag is required because ccn-lite internally increases the faceid for each management operation and interest it receives.
   * The faceid is only written to the ccnlite output, which we currently do not parse during runtime.
   * Therefore, as soon as one interest is sent, connecting nodes (= add faces to ccnlite) is no longer valid.
   * This can be removed by either parsing the ccnlite output or if ccnlite changes how faces are setup.
   */
  private var isConnecting = true

  private var isDoingManagementOperations = false

  val maybeNFNServer: Option[ActorRef] = {
    maybeComputeNodeConfig map { computeNodeConfig =>
      val system = ActorSystem(s"Sys${computeNodeConfig.prefix.toString.replace("/", "-")}", AkkaConfig.config(StaticConfig.debugLevel))
      NFNServerFactory.nfnServer(system, routerConfig, computeNodeConfig)
    }
  }

  val ccnLiteProcess: CCNLiteProcess = {
    val ccnLiteNFNNetworkProcess: CCNLiteProcess = CCNLiteProcess(routerConfig)
    ccnLiteNFNNetworkProcess.start()

    // If there is also a compute server, setup the local face
    maybeComputeNodeConfig map {computeNodeConfig =>
      if(!routerConfig.isCCNOnly) {
        ccnLiteNFNNetworkProcess.addPrefix(CCNName("COMPUTE"), computeNodeConfig.host, computeNodeConfig.port)
        //          ccnLiteNFNNetworkProcess.addPrefix(computeNodeConfig.prefix, computeNodeConfig.host, computeNodeConfig.port)
        Monitor.monitor ! Monitor.ConnectLog(computeNodeConfig.toNodeLog, routerConfig.toNodeLog)
        Monitor.monitor ! Monitor.ConnectLog(routerConfig.toNodeLog, computeNodeConfig.toNodeLog)
      }
    }
    ccnLiteNFNNetworkProcess
  }



  private def nfnMaster = {
    assert(isRunning, "Node was already shutdown")
    assert(maybeNFNServer.isDefined, "Node received command which requires a local nfn server, init the node with a RouterConfig")
    maybeNFNServer.get
  }

  /**
   * Sets up ccn-lite face to another node.
   * A ccn-lite face actually consists of two faces:
   * - a network face (currently UDP)
   * - and registration of the prefix und the face (there could be several prefixes under the same network face, but currently only one is supported)
   * @param otherNode
   */
  def connect(otherNode: LocalNode) = {
    assert(isConnecting, "Node can only connect to other nodes before caching any content")

    Monitor.monitor ! Monitor.ConnectLog(routerConfig.toNodeLog, otherNode.routerConfig.toNodeLog)
    ccnLiteProcess.connect(otherNode.routerConfig)
  }

  def addNodeFace(faceOfNode: LocalNode, gateway: LocalNode) = {
    addPrefixFace(faceOfNode.routerConfig.prefix, gateway)
  }

  def addPrefixFace(prefix: CCNName, gateway: LocalNode) = {
    val gatewayConfig = gateway.routerConfig
    ccnLiteProcess.addPrefix(prefix, gatewayConfig.host, gatewayConfig.port)
  }

  def addNodeFaces(faceOfNodes: List[LocalNode], gateway: LocalNode) = {
    faceOfNodes map { addNodeFace(_, gateway) }
  }

  /**
   * Connects this with other and other with this, see [[connect]] for details of the connection process.
   * @param otherNode
   */
  def connectBidirectional(otherNode: LocalNode) = {
    this.connect(otherNode)
    otherNode.connect(this)
  }

  /**
   * Connects otherNode with this, see [[connect]] for details of the connection process
   * @param otherNode
   */
  def connectFromOther(otherNode: LocalNode) = otherNode.connect(this)

  /**
   * Symbolic method for [[connectBidirectional]]
   * @param otherNode
   */
  def <~>(otherNode: LocalNode) =  connectBidirectional(otherNode)

  /**
   * Symbolic method for [[connect]]
   * @param otherNode
   */
  def ~>(otherNode: LocalNode) = connect(otherNode)

  /**
   * Symbolic methdo for [[connectFromOther]]
   * @param otherNode
   */
  def <~(otherNode: LocalNode) = connectFromOther(otherNode)


  /**
   * Caches the given content in the node.
   * @param content
   */
  def cache(content: Content) = {
    if(isConnecting)

    nfnMaster ! NFNApi.AddToCCNCache(content)
  }

  /**
   * Symoblic methdo for [[cache]]
   * @param content
   */
  def +=(content: Content) = cache(content)

  /**
   * Advertises the services to the network.
   * TODO: this should be changed to advertise the service by setting up faces instead of adding them to the cache
   */
  def publishServices = NFNServiceLibrary.nfnPublish(nfnMaster)

  def publishService(serv: NFNService) = {
    NFNServiceLibrary.nfnPublishService(serv, nfnMaster)
  }

  def removeLocalServices = NFNServiceLibrary.removeAll()

  /**
   * Fire and forgets an interest to the system. Response will still arrive in the localAbstractMachine cache, but will discarded when arriving
   * @param req
   */
  def send(req: Interest)(implicit useThunks: Boolean) = {
    isConnecting = false
    nfnMaster ! NFNApi.CCNSendReceive(req, useThunks)
  }

  /**
   * Sends the request and returns the future of the content
   * @param req
   * @return
   */
  def sendReceive(req: Interest)(implicit useThunks: Boolean): Future[Content] = {
    (nfnMaster ? NFNApi.CCNSendReceive(req, useThunks)).mapTo[CCNPacket] map {
      case n: NAck => throw new Exception(":NACK")
      case c: Content => c
      case i: Interest => throw new Exception("An interest was returned, this should never happen")
    }
  }


  /**
   * Symbolic method for [[send]]
   * @param req
   */
  def !(req: Interest)(implicit useThunks: Boolean) = send(req)

  /**
   * Symbolic method for [[sendReceive]]
   * @param req
   * @return
   */
  def ?(req: Interest)(implicit useThunks: Boolean): Future[Content] = sendReceive(req)

  /**
   * Shuts this node down. After shutting down, any method call will result in an exception
   */
  def shutdown() = {
    assert(isRunning, "This node was already shut down")
    nfnMaster ! NFNServer.Exit()
    ccnLiteProcess.stop()
    isRunning = false
  }

}
