package nfn

import nfn.service._
import lambdacalculus.machine._


case class NFNValueToMachineValu() {
  def toMachineValue(nfnValue: NFNValue):Value =  {

    if(nfnValue.isInstanceOf[NFNListValue]) {
      val lv = nfnValue.asInstanceOf[NFNListValue]

      val r: Seq[Value] =
        for { v <- lv.values}
        yield { toMachineValue(v) }

      val asdf: ListValue = ListValue(r, maybeContextName = None)
      println(asdf)

      ConstValue(4)
    } else if(nfnValue.isInstanceOf[NFNIntValue]) {
      val n = nfnValue.asInstanceOf[NFNIntValue].amount
      ConstValue(n)

    } else {
      ConstValue(5)
    }
  }

//    nfnValue match {
//    case NFNIntValue(n) => ConstValue(n)
//    //      case NFNNameValue(name) => VariableValue(name.toString)
//    case NFNListValue(values) => {
//      val list: List[Value] = values map { toMachineValue(_) }
//      ListValue(list)
//    }
//    case _ =>  throw new Exception(s"NFNValueToMachineValue: conversion of $nfnValue to machine value type not implemented")
//  }
}

