//package nfn
//
//import akka.actor.Actor
//import akka.actor.Actor.Receive
//import ccn.packet._
//import lambdacalculus._
//
//class AbstractMachineWorker extends Actor {
//
//  val executor =
//  val lc = LambdaCalculus(ExecutionOrder.CallByValue, debug = true, storeIntermediateSteps = true, Some(executor))
//
//  def handleInterest(interest: Interest): Unit = {
//
//  }
//
//  override def receive: Receive = {
//    case packet: CCNPacket => packet match {
//      case i: Interest => handleInterest(i)
//      case c: Content =>
//    }
//  }
//
//}
