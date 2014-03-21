
import com.typesafe.scalalogging.slf4j.Logging
import scala.concurrent._
import ExecutionContext.Implicits.global
import java.net.{DatagramPacket, InetAddress, DatagramSocket}

case class UDPClient(name: String, port: Int, maybeAddr: Option[InetAddress] = None) extends Logging {

  logger.debug(s"$name: opened socket at${maybeAddr.getOrElse("localhost")} $port")

  val addr = maybeAddr.getOrElse(InetAddress.getByName(null))
  val clientSocket = new DatagramSocket()

  def send(data: Array[Byte]): Future[Unit] = {
    future {
      val packet = new DatagramPacket(data, data.length, addr, port)
      clientSocket.send(packet)
      logger.debug(s"$name: sent data: $data")
    }
  }

  def receive(): Future[Array[Byte]] = {
    future {
      val rcvBuf = new Array[Byte](1024)
      val rcvPacket = new DatagramPacket(rcvBuf, rcvBuf.length)
      logger.debug(s"$name: waiting for receive...")
      clientSocket.receive(rcvPacket)
      logger.debug(s"$name: received: ${new String(rcvBuf)}")
      rcvBuf
    }
  }

  def sendReceive(data: Array[Byte]): Future[Array[Byte]] = send(data).flatMap(_ => receive())

  def close() = {
    clientSocket.close()
  }
}

