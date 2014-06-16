package nfn.service

import java.io.{File, FileOutputStream}

import akka.actor.ActorRef
import bytecode.BytecodeLoader
import ccn.ccnlite.CCNLite
import ccn.packet._
import com.typesafe.scalalogging.slf4j.Logging
import nfn.NFNApi

import scala.language.experimental.macros
import scala.reflect.runtime.{universe => ru}


object NFNServiceLibrary extends Logging {
  private var services:Map[CCNName, NFNService] = Map()
  val tempBytecodeContentDirName = "./temp-bytecode-content"

  def add(serv: NFNService) =  {
    val name = serv.ccnName
    services += name -> serv
  }

  def removeAll() = services = Map()

  def find(servName: String):Option[NFNService] = {

    CCNName.fromString(servName) match {
      case Some(name) =>
        val found = services.get(name)
        logger.debug(s"Found $found")
        found
      case None =>
        logger.error(s"Could not split name $servName with '/'")
        None
    }
  }


  def find(servName: CCNName):Option[NFNService] = find(servName.toString)

  def convertDollarToChf(dollar: Int): Int = ???

  def nfnPublishService(serv: NFNService, ccnWorker: ActorRef) = {
    def pinnedData = "pinnedfunction".getBytes

    def byteCodeData(serv: NFNService):Array[Byte] = {
      BytecodeLoader.fromClass(serv) match {
        case Some(bc) => {
//          if(bc.length + serv.ccnName)
          // TODO remove this when packet fragmentation works
          // replace with simply 'bc'
          val tempBytecodeContentDir = new File(tempBytecodeContentDirName)
          if(!tempBytecodeContentDir.exists()) {
            tempBytecodeContentDir.mkdir()
          }
          val bytecodeContentFileName = s"$tempBytecodeContentDirName/${serv.ccnName.cmps.mkString("+")}"

          val bytecodeContentFile = new File(bytecodeContentFileName)
          if(bytecodeContentFile.exists()) {
            bytecodeContentFile.delete()
            bytecodeContentFile.createNewFile()
          }
          val out = new FileOutputStream(bytecodeContentFile)
          try{
            out.write(bc)
          } finally { out.close() }

          bytecodeContentFileName.getBytes
        }
        case None =>
          logger.error(s"nfnPublush: No bytecode found for unpinned service $serv")
          pinnedData
      }
    }

    val serviceContent =
      if(serv.pinned) pinnedData
      else byteCodeData(serv)

    val content = Content(
      serv.ccnName,
      serviceContent
    )

    logger.debug(s"nfnPublish: Adding ${content.name} (size=${serviceContent.size}) to cache")
    ccnWorker ! NFNApi.AddToCCNCache(content)
  }

  /**
   * Advertises all locally available services to nfn by sending a 'addToCache' Interest,
   * containing a content object with the name of the service (and optionally also the bytecode * of the service).
   * @param ccnWorker
   */
  def nfnPublish(ccnWorker: ActorRef) = {
    for((_, serv) <- services) {
      nfnPublishService(serv, ccnWorker)
    }
  }

  def convertChfToDollar(chf: Int): Int = ???
  def toPdf(webpage: String): String = ???
  def derp(foo: Int) = ???
}

case class NFNServiceExecutionException(msg: String) extends Exception(msg)
case class NFNServiceArgumentException(msg: String) extends Exception(msg)

case class CallableNFNService(name: CCNName, values: Seq[NFNValue], nfnMaster: ActorRef, function: (Seq[NFNValue], ActorRef) => NFNValue, executionTimeEstimate: Option[Int]) extends Logging {
  def exec:NFNValue = function(values, nfnMaster)
}

object Main {

  def bytecodeLoading = {
    val jarfile = "/Users/basil/Dropbox/uni/master_thesis/code/testservice/target/scala-2.10/testservice_2.10-0.1-SNAPSHOT.jar"
    val service = BytecodeLoader.loadClass[NFNService](jarfile, "NFNServiceTestImpl")
  }

  def findLibraryFunctionWithReflection = {
    def getTypeTag[T: ru.TypeTag](obj: T) = ru.typeTag[T]

    val tpe = getTypeTag(NFNServiceLibrary).tpe

    val methods: List[String] =
      tpe.declarations.filter(decl =>
        decl.isMethod && decl.name.toString != "<init>"
      ).map( methodDecl => {
        def parseTypeString(tpe: String): String = {
          val splitted = tpe.substring(1, tpe.size).split("\\)")
          val params:List[String] = splitted(0).split(", ").map(param => param.substring(param.indexOf(": ")+2, param.size)).toList
          val ret = splitted(1).trim
          s"${params.mkString("/")}/r$ret"
        }
        val name = methodDecl.name.toString
        val tpes = parseTypeString(methodDecl.typeSignature.toString)
        //        typeSignature.map( (tpe: ru.Type) => {
        ////          tpe.toString
        ////        }
        ////        )
        s"/$name/$tpes"
      }
        ).toList

    println(methods.mkString("\n"))
  }
}

