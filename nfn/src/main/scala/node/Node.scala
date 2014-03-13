package node

import nfn.{NFNMaster, NFNMasterFactory, NodeConfig}
import scala.concurrent.duration.FiniteDuration
import akka.util.Timeout
import akka.actor.ActorSystem
import config.AkkaConfig
import nfn.NFNMaster.{Exit, Connect}
import ccn.packet.{Interest, Content}
import scala.concurrent.Future

/**
 * Created by basil on 10/04/14.
 */
case class Node(nodeConfig: NodeConfig) {

  val timeoutDuration: FiniteDuration = 6 seconds
  implicit val timeout = Timeout( timeoutDuration)

  private var isRunning = true
  private var isConnecting = true

  private val system = ActorSystem("NFNActorSystem1", AkkaConfig())
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
    nfnMaster ! Connect(otherNode.nodeConfig)
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
    nfnMaster ! NFNMaster.CCNAddToCache(content)
  }

  /**
   * Symoblic methdo for [[cache]]
   * @param content
   */
  def +=(content: Content) = cache(content)

  /**
   * Fire and forgets an interest to the system. Response will still arrive in the local cache, but will discarded when arriving
   * @param req
   */
  def send(req: Interest) = {
    nfnMaster ! NFNMaster.CCNSendReceive(req)
  }

  /**
   * Sends the request and returns the future of the content
   * @param req
   * @return
   */
  def sendReceive(req: Interest): Future[Content] = {
    (nfnMaster ? NFNMaster.CCNSendReceive(req)).mapTo[Content]
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
    isRunning = false
    nfnMaster ! Exit()
    system.shutdown
  }

}
