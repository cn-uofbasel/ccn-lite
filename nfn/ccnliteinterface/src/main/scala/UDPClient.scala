
import scala.concurrent._
import ExecutionContext.Implicits.global
import java.net.{DatagramPacket, InetAddress, DatagramSocket}

case class UDPClient(port: Int = 9000, maybeAddr: Option[InetAddress] = None) {

  val addr = maybeAddr.getOrElse(InetAddress.getByName(null))
  val clientSocket = new DatagramSocket()

  def send(data: Array[Byte]): Future[Unit] = {
    future {
      val packet = new DatagramPacket(data, data.length, addr, port)
      clientSocket.send(packet)
      println(s"sent: $data")
    }
  }

  def receive(): Future[Array[Byte]] = {
    future {
      val rcvBuf = new Array[Byte](1024)
      val rcvPacket = new DatagramPacket(rcvBuf, rcvBuf.length)
      println("waiting for response...")
      clientSocket.receive(rcvPacket)
      println(s"received $rcvBuf")
      rcvBuf
    }
  }

  def sendReceive(data: Array[Byte]): Future[Array[Byte]] = send(data).flatMap(_ => receive())

  def close() = {
    clientSocket.close()
  }
}

