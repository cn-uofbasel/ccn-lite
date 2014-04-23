package ccn

import java.io._
import com.typesafe.scalalogging.slf4j.Logging
import nfn.NodeConfig

/**
 * Pipes a [[InputStream]] to a file with the given name into ./log/<name>.log.
 * If you don't want the logfile to be overridden, set appendSystemTime to true.
 * @param is input stream to pipe the content to a file
 * @param logname the name of the file (./log/logname)
 * @param appendTimestamp Avoids overriding of the file by writing to ./log/<logname>-timestamp.log instead
 */
class LogStreamReaderToFile(is: InputStream, logname: String, appendTimestamp: Boolean = false) extends Runnable {

  private val reader = new BufferedReader(new InputStreamReader(is))
  val filename = {
    if (appendTimestamp) s"./log/$logname.log"
    else s"./log/$logname-${System.currentTimeMillis}.log"
  }

  private val writer = new BufferedWriter(new FileWriter(new File(filename)))

  override def run() = {
    try {
      var line = reader.readLine
      while (line != null) {
        writer.write(line + "\n")
        line = reader.readLine
      }
    } finally {
      reader.close()
      writer.close()
    }
  }
}

/**
 * Encapsulates a native ccn-lite-relay process.
 * Starts a process with the given port and sets up a compute port. All output is written to a file in './log/ccnlite-<host>-<port>.log.
 * Call start and stop.
 * @param nodeConfig
 */
case class CCNLiteProcess(nodeConfig: NodeConfig) {

  private var process: Process = null
  private var faceId = 2

  val host = nodeConfig.host
  val port = nodeConfig.port
  val computePort = nodeConfig.computePort
  val prefix = nodeConfig.prefix

  val sockName = s"/tmp/mgmt.$prefix.sock"


  def start() = {
//    if(nodeConfig.prefix != "docRepo0" && nodeConfig.prefix != "docRepo1") {
      val cmd = s"../ccn-lite-relay -v 99 -u $port -x $sockName"
      println(s"CCNLiteProcess-$prefix: executing: '$cmd'")
      val processBuilder = new ProcessBuilder(cmd.split(" "): _*)
      processBuilder.redirectErrorStream(true)
      process = processBuilder.start

      val lsr = new LogStreamReaderToFile(process.getInputStream, s"ccnlite-$host-$port", appendTimestamp = true)
      val thread = new Thread(lsr, s"LogStreamReader-$prefix")
      thread.start
//    } else {
//      println("SKIPPED START OF CCN-LITE NODE 0")
//      println("SKIPPED START OF CCN-LITE NODE 1")
//    }

    addFace(host, computePort, "COMPUTE")
    faceId = 5
  }

  def stop() = {
    println(s"CCNLiteProcess-$prefix: stop")
    if (process != null) {
      process.destroy()
    }
  }

  def addFace(otherHost: String, otherPort: Int, otherPrefix: String): Unit = {

//    val cmdUDPFace = s"../util/ccn-lite-ctrl -x $sockName newUDPface any $otherHost $otherPort"
    // TODO host is hardcoded!
    val cmdUDPFace = s"../util/ccn-lite-ctrl -x $sockName newUDPface any 127.0.0.1 $otherPort"
    println("TODO: host is still hardcoded")
    println(s"CCNLiteProcess-$prefix: executing '$cmdUDPFace")
    Runtime.getRuntime.exec(cmdUDPFace.split(" "))

    val cmdPrefixReg =  s"../util/ccn-lite-ctrl -x $sockName prefixreg /$otherPrefix $faceId"
    println(s"CCNLiteProcess-$prefix: executing '$cmdPrefixReg")
    Runtime.getRuntime.exec(cmdPrefixReg.split(" "))
    faceId += 3
  }

  def connect(otherNodeConfig: NodeConfig): Unit = {
    println(s"Connecting $nodeConfig to $otherNodeConfig")
    addFace(otherNodeConfig.host, otherNodeConfig.port, otherNodeConfig.prefix)
  }
}
