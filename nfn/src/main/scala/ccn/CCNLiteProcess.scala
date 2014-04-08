package ccn

import java.io._
import com.typesafe.scalalogging.slf4j.Logging

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
 * @param host host, this will usually be localhost
 * @param ccnLitePort the main ccn-lite port
 * @param computePort the compute port where computation requests are sent to.
 */
case class CCNLiteProcess(host: String, ccnLitePort: Int, computePort: Int) extends Logging {

  private var process: Process = null

  def start() = {
    logger.info("starting...")
    val processBuilder = new ProcessBuilder(s"../ccn-lite-relay -v 99 -u $ccnLitePort -p /tmp/mgmt.sock".split(" "):_*)
    processBuilder.redirectErrorStream(true)

    val pb1 = new ProcessBuilder(s"../util/ccn-lite-ctrl -x /tmp/mgmt.sock newUDPface any 127.0.0.1 $computePort".split(" "):_*)
    val pb2 = new ProcessBuilder(s"../util/ccn-lite-ctrl -x /tmp/mgmt.sock prefixreg /COMPUTE 3".split(" "):_*)

    process = processBuilder.start
    pb1.start
    pb2.start

    val lsr = new LogStreamReaderToFile(process.getInputStream, s"ccnlite-$host-$ccnLitePort", appendTimestamp = true)
    val thread = new Thread(lsr, "LogStreamReader")
    thread.start
  }

  def stop() = {
    logger.info("stop")
    if (process != null) {
      process.destroy()
    }
  }
}
