package ccnliteinterface.cli

import java.io._

import ccnliteinterface._
import com.typesafe.scalalogging.slf4j.Logging

import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global

/**
 * Created by basil on 03/09/14.
 */
case class CCNLiteInterfaceCli(wireFormat: CCNLiteWireFormat) extends CCNLiteInterface with Logging {

  val utilFolderName = "../util/"

  private def executeCommandToByteArray(cmds: Array[String], maybeDataToPipe: Option[Array[Byte]]): (Array[Byte], Array[Byte]) = {

    def futInToOut(is: InputStream, os: OutputStream): Future[Unit] = {
      Future(
        Iterator.continually(is.read)
                .takeWhile(-1 !=)
                .foreach(os.write)
      )
    }
    val rt = Runtime.getRuntime

//    logger.debug(s"Executing command: ${cmds.mkString(" ")}")
    val proc = rt.exec(cmds)

    val procIn = new BufferedInputStream(proc.getInputStream)
    val resultOut = new ByteArrayOutputStream()
    val futResult = futInToOut(procIn, resultOut)

    val procErrIn = new BufferedInputStream(proc.getErrorStream)
    val errorOut = new ByteArrayOutputStream()
    val futError = futInToOut(procErrIn, errorOut)

    maybeDataToPipe map { dataToPipe =>
      val procOut = new BufferedOutputStream(proc.getOutputStream)
      procOut.write(dataToPipe)
      procOut.close()
    }

    proc.waitFor()

    while(! (futResult.isCompleted && futError.isCompleted)) {
      Thread.sleep(20)
    }

    val result = resultOut.toByteArray
    val err = errorOut.toByteArray


//    logger.debug(s"Res (ret=${proc.exitValue()},size=${result.size}): ${new String(result)}")
    if(err.size > 0) {
      logger.error(s"${cmds.mkString(" ")}: ${new String(err)} (size=${err.size})")
    }
    proc.destroy()

    (result, err)
  }
  def wireFormatNum(f: CCNLiteWireFormat) = wireFormat match {
    case CCNBWireFormat() => 0
    case NDNTLVWireFormat() => 2
  }

  override def mkBinaryInterest(nameCmps: Array[String]): Array[Byte] = {
    val mkI = "ccn-lite-mkI"
    val cmds = Array(utilFolderName+mkI, "-s", wireFormatNum(wireFormat).toString, nameCmps.mkString("|"))
    val (res, _) = executeCommandToByteArray(cmds, None)
    res
  }

  override def mkBinaryContent(name: Array[String], data: Array[Byte]): Array[Byte] = {
    val mkC = "ccn-lite-mkC"
    val cmds = Array(utilFolderName+mkC, "-s", wireFormatNum(wireFormat).toString, name.mkString("|"))
    val (res, _) = executeCommandToByteArray(cmds, Some(data))

    res
  }

  override def ccnbToXml(binaryPacket: Array[Byte]): String = {
    val pktdump = "ccn-lite-pktdump"
    val cmds = Array(utilFolderName+pktdump, "-f", "1")//"-s", wireFormatNum(wireFormat).toString)

    val (res, _) = executeCommandToByteArray(cmds, Some(binaryPacket))

    new String(res)
  }


  override def mkAddToCacheInterest(ccnbAbsoluteFilename: String): Array[Byte] = {
    val ctrl = "ccn-lite-ctrl"

    val cmds = Array(utilFolderName+ctrl, "-m", "addContentToCache", ccnbAbsoluteFilename)

    val (res, _) = executeCommandToByteArray(cmds, None)

    res
  }
}

object CCNLiteInterfaceCli extends App {
  val ccnIf = CCNLiteInterfaceCli(NDNTLVWireFormat())

  val bi = ccnIf.mkBinaryInterest(Array("yay", "wo"))

  val bc = ccnIf.mkBinaryContent(Array("yay", "works"), "testdata".getBytes)

  ccnIf.ccnbToXml(bi)
  ccnIf.ccnbToXml(bc)



  val filename = "tempfile"
  val file = new File(filename)

  if(file.exists()) {
    file.delete()
  }

  file.createNewFile()

  val pw = new PrintWriter(file)
  try {
    pw.println("testcontentyo")
  }  finally {
    pw.close()
  }

  ccnIf.mkAddToCacheInterest(file.getCanonicalPath)

  file.delete()

}