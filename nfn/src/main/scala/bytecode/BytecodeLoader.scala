package bytecode

import java.net.URLClassLoader
import scala.io.Source
import java.io.File
import java.nio.file.{Paths, Path, Files}

object BytecodeLoader {
  def loadClassFromJar[T](jarfile: String, className: String): T = {
    val urls = Array(new File(jarfile).toURL)
    val cl = new URLClassLoader(urls)
    val clazz = cl.loadClass(className)
    clazz.newInstance.asInstanceOf[T]
  }

  def jarToByteArray(jarfile: String): Array[Byte] = Source.fromFile(jarfile).getLines.mkString.getBytes

  def classfileOfClass(clazz: Any):Option[File] = {
    def classToFilename(clazz: Any) = {
      def classBaseDirectory =  clazz.getClass.getProtectionDomain.getCodeSource.getLocation.getFile
      def classNamespaceDirectoriesAndName = clazz.getClass.getName.replace(".", "/")
      classBaseDirectory + classNamespaceDirectoriesAndName + ".class"
    }
    val classFile = new File(classToFilename(clazz))
    if(!classFile.exists) None
    else Some(classFile)
  }

  def fromClass(clazz: Any):Option[Array[Byte]] = {
    classfileOfClass(clazz) map { classFile =>
      val path = Paths.get(classFile.toURI)
      Files.readAllBytes(path)
    }
  }
}
