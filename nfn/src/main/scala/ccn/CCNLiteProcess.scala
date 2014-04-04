package ccn

import java.io._


class LogStreamReaderToFile(is: InputStream, logname: String, doOverride: Boolean) extends Runnable {

  private val reader = new BufferedReader(new InputStreamReader(is))
  val filename = {
    if (doOverride) s"./log/$logname.log"
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
 * Created by basil on 04/04/14.
 */
object CCNLiteProcess {

  var process: Process = null

  def start() = {
    println("starting ccn-lite process")
    val processBuilder = new ProcessBuilder("../ccn-lite-relay -v 99 -u 9000 -p /tmp/mgmt.sock".split(" "):_*)
    processBuilder.redirectErrorStream(true)

    val pb1 = new ProcessBuilder("../util/ccn-lite-ctrl -x /tmp/mgmt.sock newUDPface any 127.0.0.1 9001".split(" "):_*)
    val pb2 = new ProcessBuilder("../util/ccn-lite-ctrl -x /tmp/mgmt.sock prefixreg /COMPUTE 3".split(" "):_*)

    process = processBuilder.start

    val lsr = new LogStreamReaderToFile(process.getInputStream, "ccn-lite", doOverride = true)
    val thread = new Thread(lsr, "LogStreamReader")
    thread.start

    Thread.sleep(10)
    val tp1 = pb1.start
    println("waiting for setting up udpface")
    tp1.waitFor()
    println("waiting for registering compute socket")
    pb2.start
    println("Initialized ccn-lite!")

  }

  def stop() = {
    println("stopping ccn-lite process")
    if (process != null) {
      process.destroy()
    }
  }
}
