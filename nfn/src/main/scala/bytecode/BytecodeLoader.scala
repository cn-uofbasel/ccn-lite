package bytecode

import java.net.{URL, URLClassLoader}
import java.util.zip.ZipEntry
import org.apache.bcel.Repository
import org.apache.bcel.classfile._

import scala.io.Source
import scala.collection.convert.Wrappers.JEnumerationWrapper
import java.io.{FileOutputStream, ByteArrayOutputStream, File}
import java.nio.file.{Paths, Path, Files}
import scala.util._
import java.util.jar.{JarOutputStream, JarEntry, JarFile}
import nfn.NFNServer
import nfn.service.NFNService

object BytecodeLoader {

  class DependencyEmitter(javaClass: JavaClass) extends EmptyVisitor {
    override def visitConstantClass(obj: ConstantClass) {
      val cp = javaClass.getConstantPool
      val bytes = obj.getBytes(cp)
      System.out.println(s"found: $bytes")
    }
  }

  def byteCodeForClassAndDependencies(className: String) = {

    //    val filename = "/tmp/foo.jar"
    //    val file = new File(filename)
    //    if(file.exists()) {
    //      file.delete()
    //    }

    val baOut = new ByteArrayOutputStream()
    val jarOut = new JarOutputStream(baOut)

    val startsWithFilters = List("scala", "java", "[Ljava")
    val containFilters = List("impl")
    var folders: Set[String] = Set()
    try {
      val javaClass = Repository.lookupClass(className)
      val visitor = new EmptyVisitor() {
        override def visitConstantClass(obj: ConstantClass) {
          val cp = javaClass.getConstantPool
          val dependentClassnameQualified = obj.getBytes(cp)



          // dependantClassQualified does not start with any element of the filters
          if(startsWithFilters.forall(!dependentClassnameQualified.startsWith(_)) &&
            containFilters.forall(dependentClassnameQualified.contains)) {

            val lastSlashIndex = dependentClassnameQualified.lastIndexOf("/") + 1 // "asdf/asdf/asdf" => ("asdf/asdf/", "asdf")
            val (path, _) = dependentClassnameQualified.splitAt(lastSlashIndex)

            if (!folders.contains(path)) {
              println(s"adding folder: $path")
              folders += path
              println(s"folders: $folders")
              jarOut.putNextEntry(new ZipEntry(path)); // Folders must end with "/".
            }
            println(s"adding: $dependentClassnameQualified.class")
            jarOut.putNextEntry(new ZipEntry(dependentClassnameQualified + ".class"))
            jarOut.write(javaClass.getBytes)
            println(s"adding bytecode: ${new String(javaClass.getBytes).take(50)} ...")
            jarOut.closeEntry()
          } else {
            println(s"skipping: $dependentClassnameQualified")
          }
        }
      }
      val classWalker = new DescendingVisitor(javaClass, visitor)
      classWalker.visit()

    } finally {
      jarOut.close()
    }

    baOut.toByteArray
  }



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
//    classfileOfClass(clazz) map { classFile =>
//      val path = Paths.get(classFile.toURI)
//      Files.readAllBytes(path)
//    }
    val byteCode = byteCodeForClassAndDependencies(clazz.getClass.getCanonicalName)
    println(s"loaded bytecode (size: ${byteCode.size}")
    Some(byteCode)

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

