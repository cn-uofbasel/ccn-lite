package bytecode

import java.net.{URL, URLClassLoader}
import scala.io.Source
import scala.collection.convert.Wrappers.JEnumerationWrapper
import java.io.File
import java.nio.file.{Paths, Path, Files}
import scala.util._
import java.util.jar.{JarEntry, JarFile}
import nfn.NFNServer
import nfn.service.NFNService

object BytecodeLoader {


  /**
   * Loads a class from either from:
   * - a directory containing .class files
   * - from a jarfile (containing .class files)
   * The result is casted to the type parameter T
   * @param dirOrJar Either a directory containing .class files or a jarfile
   * @param className Name of the class to be loaded (don't forget package names)
   * @tparam T Type the loaded class is casted to
   * @return
   */
  def loadClass[T](dirOrJar: String, classNameToLoad: String): Try[T] = {
    val url = new File(dirOrJar).toURI.toURL
//    val urls = Array(url)

    val jar = new JarFile(dirOrJar)

    val entries: Iterator[JarEntry] = JEnumerationWrapper(jar.entries())

    val urls: Array[URL] = Array( new URL(s"jar:file:$dirOrJar!/") )

    val cl: URLClassLoader = new URLClassLoader(urls)

    entries foreach { e =>
      if (!e.isDirectory && e.getName.endsWith(".class")) {
        println(s"Class entry: ${e.getName}")
        val classEntryName = e.getName.substring(0, e.getName.length-6).replace('/', '.')
        if(classEntryName == classNameToLoad.replace("/", "").replace("_", ".")) {
          val clazz = cl.loadClass(classEntryName)

          val loadedClazz: T = clazz.newInstance.asInstanceOf[T]
          return Try(loadedClazz)
        }
      }
    }
    throw new Exception(s"Class $classNameToLoad was not found in jar")
  }

  def jarToByteArray(jarfile: String): Array[Byte] = Source.fromFile(jarfile).getLines.mkString.getBytes

  def classToFilename(clazz: Any) = {
    def classBaseDirectory =  clazz.getClass.getProtectionDomain.getCodeSource.getLocation.getFile
    def classNamespaceDirectoriesAndName = clazz.getClass.getName.replace(".", "/")

    classBaseDirectory + classNamespaceDirectoriesAndName + ".class"
  }
  def classToDirname(clazz: Any) = {
    clazz.getClass.getProtectionDomain.getCodeSource.getLocation.getFile
  }

  def classfileOfClass(clazz: Any):Option[File] = {
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

  def toClass[T](classBytecode: Array[Byte], className: String) = {
    val cl = this.getClass.getClassLoader
    val clazz = cl.loadClass(className)
    clazz.newInstance.asInstanceOf[T]
  }
}



//object Test extends App {
//  import BytecodeLoader._
//
//
//  val classDir = classToDirname(new TestClass())
//  loadClass[TestClass](classDir, TestClass().getClass.getCanonicalName) match {
//    case Success(tc: TestClass) => tc.fun
//    case Failure(e) => throw e
//  }
//}

