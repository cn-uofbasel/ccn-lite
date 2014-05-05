package node

import nfn.{NFNApi, NFNMaster, NFNMasterFactory, NodeConfig}
import scala.concurrent.duration.FiniteDuration
import akka.util.Timeout
import akka.actor.ActorSystem
import akka.pattern._
import config.AkkaConfig
import ccn.packet.{Interest, Content}
import scala.concurrent.Future
import scala.concurrent.duration._
import nfn.service.NFNServiceLibrary
import scala.collection.immutable.{Iterable, IndexedSeq}


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
case class Node(nodeConfig: NodeConfig) {

  val timeoutDuration: FiniteDuration = 6 seconds
  implicit val timeout = Timeout( timeoutDuration)

  private var isRunning = true
  private var isConnecting = true

  private val system = ActorSystem(s"Sys${nodeConfig.prefix.replace("/", "-")}", AkkaConfig.configDebug)
  private val _nfnMaster = NFNMasterFactory.network(system, nodeConfig)

  private def nfnMaster = {
    assert(isRunning, "Node was already shutdown")
    _nfnMaster
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
    nfnMaster ! NFNApi.Connect(otherNode.nodeConfig)
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
    isConnecting = false
    nfnMaster ! NFNApi.CCNAddToCache(content)
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
   * Fire and forgets an interest to the system. Response will still arrive in the local cache, but will discarded when arriving
   * @param req
   */
  def send(req: Interest) = {
    nfnMaster ! NFNApi.CCNSendReceive(req)
  }

  /**
   * Sends the request and returns the future of the content
   * @param req
   * @return
   */
  def sendReceive(req: Interest): Future[Content] = {
    (nfnMaster ? NFNApi.CCNSendReceive(req, useThunks = true)).mapTo[Content]
  }


  /**
   * Symbolic method for [[send]]
   * @param req
   */
  def !(req: Interest) = send(req)

  /**
   * Symbolic method for [[sendReceive]]
   * @param req
   * @return
   */
  def ?(req: Interest): Future[Content] = sendReceive(req)

  /**
   * Shuts this node down. After shutting down, any method call will result in an exception
   */
  def shutdown() = {
    assert(isRunning, "This node was already shut down")
    nfnMaster ! NFNMaster.Exit()
    isRunning = false
  }

}
