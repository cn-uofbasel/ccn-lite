package node

import nfn.{NFNApi, NFNServer, CCNServerFactory, NodeConfig}
import scala.concurrent.duration.FiniteDuration
import akka.util.Timeout
import akka.actor.{ActorRef, ActorSystem}
import akka.pattern._
import config.AkkaConfig
import ccn.packet.{CCNName, Interest, Content}
import scala.concurrent.Future
import scala.concurrent.duration._
import nfn.service.NFNServiceLibrary
import scala.collection.immutable.{Iterable, IndexedSeq}
import ccn.CCNLiteProcess
import monitor.Monitor


object Node {
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
  def connectGrid(nodes: Seq[Node]): Unit = {
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
  def connectStar(nodes: Seq[Node]): Unit = {
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
  def connectAll(unconnectedNodes: Seq[Node]): Unit = {
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
  def connectLine(nodes: Seq[Node]): Unit = {
    if(nodes.size <= 1) return

    nodes.tail.foldLeft(nodes.head) {
      (left, right) => left <~> right; right
    }
  }
}

/**
 * Created by basil on 10/04/14.
 */
case class Node(nodeConfig: NodeConfig, withLocalAM: Boolean = false) {

  val timeoutDuration: FiniteDuration = 6.seconds
  implicit val timeout = Timeout( timeoutDuration)

  private var isRunning = true

  /**
   * This flag is required because ccn-lite internally increases the faceid for each interest it receives.
   * The faceid is only written to the ccnlite output, which we currently do not parse during runtime.
   * Therefore, as soon as one interest is sent, connecting nodes (= add faces to ccnlite) is no longer valid.
   * This can be removed by either parsing the ccnlite output or if ccnlite changes how faces are setup.
   */
  private var isConnecting = true

  val system = ActorSystem(s"Sys${nodeConfig.prefix.toString.replace("/", "-")}", AkkaConfig.configDebug)
  val _nfnServer: ActorRef = CCNServerFactory.ccnServer(system, nodeConfig)

  private val ccnLiteNFNNetworkProcess = CCNLiteProcess(nodeConfig)
  ccnLiteNFNNetworkProcess.start()

  ccnLiteNFNNetworkProcess.addPrefix(CCNName("COMPUTE"), nodeConfig.host, nodeConfig.computePort)
  ccnLiteNFNNetworkProcess.addPrefix(nodeConfig.prefix, nodeConfig.host, nodeConfig.computePort)

  Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toComputeNodeLog, nodeConfig.toNFNNodeLog)
  Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toNFNNodeLog, nodeConfig.toComputeNodeLog)

  private def nfnMaster = {
    assert(isRunning, "Node was already shutdown")
    _nfnServer
  }

  /**
   * Sets up ccn-lite face to another node.
   * A ccn-lite face actually consists of two faces:
   * - a network face (currently UDP)
   * - and registration of the prefix und the face (there could be several prefixes under the same network face, but currently only one is supported)
   * @param otherNode
   */
  def connect(otherNode: Node) = {
    assert(isConnecting, "Node can only connect to other nodes before caching any content")
    Monitor.monitor ! Monitor.ConnectLog(nodeConfig.toNFNNodeLog, otherNode.nodeConfig.toNFNNodeLog)
    ccnLiteNFNNetworkProcess.connect(otherNode.nodeConfig)
  }

  def addNodeFace(faceOfNode: Node, gateway: Node) = {
    ccnLiteNFNNetworkProcess.addPrefix(faceOfNode.nodeConfig.prefix, gateway.nodeConfig.host, gateway.nodeConfig.port)
  }

  def addPrefixFace(prefix: CCNName, gateway: Node) = {
    ccnLiteNFNNetworkProcess.addPrefix(prefix, gateway.nodeConfig.host, gateway.nodeConfig.port)
  }

  def addNodeFaces(faceOfNodes: List[Node], gateway: Node) = {
    faceOfNodes map { addNodeFace(_, gateway) }
  }

  /**
   * Connects this with other and other with this, see [[connect]] for details of the connection process.
   * @param otherNode
   */
  def connectBidirectional(otherNode: Node) = {
    this.connect(otherNode)
    otherNode.connect(this)
  }

  /**
   * Connects otherNode with this, see [[connect]] for details of the connection process
   * @param otherNode
   */
  def connectFromOther(otherNode: Node) = otherNode.connect(this)

  /**
   * Symbolic method for [[connectBidirectional]]
   * @param otherNode
   */
  def <~>(otherNode: Node) =  connectBidirectional(otherNode)

  /**
   * Symbolic method for [[connect]]
   * @param otherNode
   */
  def ~>(otherNode: Node) = connect(otherNode)

  /**
   * Symbolic methdo for [[connectFromOther]]
   * @param otherNode
   */
  def <~(otherNode: Node) = connectFromOther(otherNode)


  /**
   * Caches the given content in the node.
   * @param content
   */
  def cache(content: Content) = {
    nfnMaster ! NFNApi.AddToLocalCache(content)
    nfnMaster ! NFNApi.AddToCCNCache(content)
    ccnLiteNFNNetworkProcess.addPrefix(content.name, nodeConfig.host, nodeConfig.port)
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

  def removeLocalServices = NFNServiceLibrary.removeAll()

  /**
   * Fire and forgets an interest to the system. Response will still arrive in the localAbstractMachine cache, but will discarded when arriving
   * @param req
   */
  def send(req: Interest)(implicit useThunks: Boolean) = {
    isConnecting = false
    nfnMaster ! NFNApi.CCNSendReceive(req, useThunks && isNFNReq(req))
  }

  def isNFNReq(req: Interest) = !req.name.cmps.forall(_!="NFN")
  /**
   * Sends the request and returns the future of the content
   * @param req
   * @return
   */
  def sendReceive(req: Interest)(implicit useThunks: Boolean): Future[Content] = {
    (nfnMaster ? NFNApi.CCNSendReceive(req, useThunks && isNFNReq(req))).mapTo[Content]
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
    ccnLiteNFNNetworkProcess.stop()
    isRunning = false
  }

}
