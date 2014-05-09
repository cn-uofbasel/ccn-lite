package evaluation.usecase.cdn

import akka.actor._
import akka.pattern._
import ccn.packet.{Interest, Content}
import nfn.NFNServer
import lambdacalculus.parser.ast.{Expr, Variable, Call}
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import akka.util.Timeout

case class CDN(nfnMaster: ActorRef) {


//  def translate(text: String, lang: String): Future[Content] = {
//    val timeout = Timeout(3000)
//    val interest = Interest(Seq(s"call 2 /nfn_service_Translate $text", "NFN"))
//    (nfnMaster ? NFNServer.CCNSendReceive(interest)).mapTo[Content]
//  }

  def loadBalance(req: String): String = {
    s"""
      |  if( call 1 /nfn_service_HasComputeAvail )
      |    $req
      |  else
      |    forward
    """.stripMargin
  }
}
