package bytecode

import java.io.ByteArrayOutputStream
import java.net.{URL, URLClassLoader}
import java.util.jar.{JarEntry, JarFile, JarOutputStream}
import java.util.zip.ZipEntry

import com.typesafe.scalalogging.slf4j.Logging
import org.apache.bcel.Repository
import org.apache.bcel.classfile._

import scala.collection.convert.Wrappers.JEnumerationWrapper
import scala.util._


/**
 * Both specified class from a jar and creates bytecode jarfile for a certain class.
 */
object BytecodeLoader extends Logging {

  class DependencyEmitter(javaClass: JavaClass) extends EmptyVisitor {
    override def visitConstantClass(obj: ConstantClass) {
      val cp = javaClass.getConstantPool
      val bytes = obj.getBytes(cp)
      System.out.println(s"found: $bytes")
    }
  }

  /**
   * Finds the class with the given class name, finds all dependent classes, filters out both Scala and Java SDK classes,
   * puts everything accordingly into a jarfile and returns the data for this jarfile
   * @param className The class to find the bytecode for all dependent classes
   * @return The data of the created jarfile
   */
  def byteCodeForClassAndDependencies(className: String): Array[Byte] = {

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
              folders += path
              jarOut.putNextEntry(new ZipEntry(path)); // Folders must end with "/".
            }
            jarOut.putNextEntry(new ZipEntry(dependentClassnameQualified + ".class"))
            jarOut.write(javaClass.getBytes)
            jarOut.closeEntry()
          }
        }
      }
      val classWalker = new DescendingVisitor(javaClass, visitor)
      classWalker.visit()

    } finally {
      jarOut.close()
    }
    logger.info(s"Created jar with dependant bytecode for class $className")

    baOut.toByteArray
  }

  /**
   * Loads the specified class form a jarfile.
   * @param jarFilename Name of the jarfile to load
   * @param classNameToLoad Name of the class that should be loaded from the jar
   * @tparam T The type of the resulting class, usually a trait the class to load implements
   * @return The laoded class of the type parameter
   */
  def loadClass[T](jarFilename: String, classNameToLoad: String): Try[T] = {

    val jar = new JarFile(jarFilename)

    val entries: Iterator[JarEntry] = JEnumerationWrapper(jar.entries())

    val urls: Array[URL] = Array( new URL(s"jar:file:$jarFilename!/") )

    val cl: URLClassLoader = new URLClassLoader(urls, this.getClass.getClassLoader)

    entries foreach { e =>

      if (!e.isDirectory && e.getName.endsWith(".class")) {
//        println(s"Class entry: ${e.getName}")


        val entryClassName = e.getName.substring(0, e.getName.length-6).replace('/', '.')

        if(entryClassName == classNameToLoad.replace("/", "").replace("_", ".")) {
          val clazz = cl.loadClass(entryClassName)

          val loadedClazz: T = clazz.newInstance.asInstanceOf[T]
          return Try(loadedClazz)
        }
      }
    }
    throw new Exception(s"Class $classNameToLoad was not found in jar")
  }

  def fromClass(clazz: Any):Option[Array[Byte]] = {
    val byteCode = byteCodeForClassAndDependencies(clazz.getClass.getName)
    Some(byteCode)
  }
}

