
import scala.concurrent._
import ExecutionContext.Implicits.global
import java.net.{DatagramPacket, InetAddress, DatagramSocket}

case class UDPClient(port: Int = 9876, maybeAddr: Option[InetAddress] = None) {

  val addr = maybeAddr.getOrElse(InetAddress.getByName(null))
  val clientSocket = new DatagramSocket()

  def send(data: Array[Byte]): Future[Unit] = {
    future {
      val packet = new DatagramPacket(data, data.length, addr, port)
      clientSocket.send(packet)
    }
  }

  def receive(): Future[Array[Byte]] = {
    future {
      val rcvBuf = new Array[Byte](1024)
      val rcvPacket = new DatagramPacket(rcvBuf, rcvBuf.length)
      clientSocket.receive(rcvPacket)
      rcvBuf
    }
  }

  def sendReceive(data: Array[Byte]): Future[Array[Byte]] = send(data).flatMap(_ => receive())

  def close() = {
    clientSocket.close()
  }
}

