package nfn.localAbstractMachine

import scala.concurrent.{Future, Await}
import scala.concurrent.duration._
import scala.util.Failure

import scala.concurrent.ExecutionContext.Implicits.global
import akka.actor.ActorRef

import lambdacalculus.machine._
import nfn.service._
import lambdacalculus.machine.CallByValue.VariableValue

case class LocalNFNCallExecutor(ccnWorker: ActorRef) extends CallExecutor {
  override def executeCall(call: String): Value = {

    val futValue: Future[Value] = {
      for {
        callableServ <- NFNService.parseAndFindFromName(call, ccnWorker)
      } yield {
        val result = callableServ.exec
        NFNValueToMachineValue.toMachineValue(result)
      }
    }
    Await.result(futValue, 20 seconds)
  }
}

object NFNValueToMachineValue {
  def toMachineValue(nfnValue: NFNValue):Value =  {

    nfnValue match {
      case NFNIntValue(n) => ConstValue(n)
      case NFNNameValue(name) => VariableValue(name.toString)
      case NFNEmptyValue() => NopValue()
      case NFNListValue(values: List[NFNValue]) => ListValue(values map { toMachineValue})
      case _ =>  throw new Exception(s"NFNValueToMachineValue: conversion of $nfnValue to machine value type not implemented")
    }
  }

}

