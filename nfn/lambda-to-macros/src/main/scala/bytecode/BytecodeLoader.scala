package bytecode

import java.net.URLClassLoader
import scala.io.Source
import java.io.File

/**
 * Created by basil on 24/02/14.
 */
object BytecodeLoader {
  def loadClassFromJar[T](jarfile: String, className: String): T = {
    val urls = Array(new File(jarfile).toURL)
    val cl = new URLClassLoader(urls)
    val clazz = cl.loadClass(className)
    clazz.newInstance.asInstanceOf[T]
  }

  def jarToByteArray(jarfile: String): Array[Byte] = Source.fromFile(jarfile).getLines.mkString.getBytes

  def getClassfileOfClass(clazz: Any):Option[File] = {
    val classFile = new File(clazz.getClass.getProtectionDomain.getCodeSource.getLocation.getFile + this.getClass.getName + ".class")
    if(!classFile.exists) None
    else Some(classFile)
  }
}
