package bytecode

import nfn.service.NFNService
import scala.util.{Success, Failure, Try}

/**
 * Created by basil on 10/06/14.
 */
object ByteLoaderTest extends App {
//  val testServiceJarfile = "./service-library/testservice_2.10-0.1-SNAPSHOT.jar"
  val testServiceJarfile = "/Users/basil/Dropbox/uni/master_thesis/ccn-lite/nfn/testservice/target/scala-2.10/testservice_2.10-0.1-SNAPSHOT.jar"

  val tryServ: Try[NFNService] = BytecodeLoader.loadClass[NFNService](testServiceJarfile, "/testservice_ChfToDollar")

  tryServ match {
    case Success(serv) => println(s"${serv.ccnName} returned success, throw argument exception"); throw serv.argumentException(Nil)

    case Failure(e) => throw e
  }
}
