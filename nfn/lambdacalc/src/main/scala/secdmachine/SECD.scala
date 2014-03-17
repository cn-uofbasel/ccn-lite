package secdmachine

import lambdacalculus.Expr

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 15:22
 * To change this template use File | Settings | File Templates.
 */
object SECD {

  class asInt(b: Boolean) {
    def toInt = if(b) 1 else 0
  }
  implicit def convertBooleanToInt(b: Boolean) = new asInt(b)

  class asBoolean(i: Int) {
    def toBool = if(i == 1) true else false
  }
  implicit def convertIntToBoolean(i: Int) = new asBoolean(i)

  class asChar(i: Int) {
    def toChar = Character.toChars(i)(0)
  }
  implicit def convertIntToChar(i: Int) = new asChar(i)


  def main(args: Array[String]) = apply

  def apply(expr: Expr) = {

  }

  def apply() {
    //    secd.code = List(Nil(),
    //      LDC(1337), Binary(Cons()),
    //      LDC(2448), Binary(Cons()),
    //      Unary(Car()))
    //    secd.code = List(LDC(5), Unary(Atom()))
    //    secd.code = List(
    //                     LDC(1), LDC(1), Binary(Sub()),
    //                     LDC(0), Binary(Eq()),
    //                     SEL(),
    //                       InstructionBlock(List( LDC(true toInt),  JOIN() )),
    //                       InstructionBlock(List( LDC(false toInt), JOIN() ))
    //                    )
    // --> true


    //    secd.code = List(
    //      NIL(), Unary(Null()),
    //      SEL(),
    //        InstructionBlock(List(LDC(10), JOIN())),
    //        InstructionBlock(List(LDC(20), JOIN())),
    //      LDC(10), Binary(Add())
    //    )




    val secd = new SECDState()
    secd.stack = List()
    secd.env = List()
    // (la x . x + 3) 4
    secd.code = List(
      NIL(),
      LDC(4),
      Binary(Cons()),
      //        DUM(),
      LDF(),
      InstructionBlock(List(
        LD(0,0),
        LDC(3), Binary(Add()),
        RTN()
      )),
      AP()
    )
    secd.dump = List()

    while(secd.code.size > 0) {
      println(secd)
      step(secd)
      println("-----stepped-----")
    }
    println(secd)
  }

  def step(state:SECDState)= {
    val stack = state.stack
    val env = state.env
    val code = state.code
    val dump = state.dump

    state.code.head match {
      case h:NIL => {
        state.stack = CList(List()) :: stack
        state.env = env
        state.code = code.drop(1)
        state.dump = dump
      }
      case h:LDC => {
        state.stack = CInteger(h.integer) :: stack
        state.env = env
        state.code = code.drop(1)
        state.dump = dump
      }
      case h:LD => {
        state.stack = env(h.x)(h.y) :: stack
        state.env = env
        state.code = code.drop(1)
        state.dump = dump
      }
      case h:Unary => {
        state.stack = applyUnary(stack.head, h.op) :: stack.drop(1)
        state.env = env
        state.code = code.drop(1)
        state.dump = dump
      }
      case h:Binary => {
        state.stack = applyBinary(stack(0), stack(1), h.op) :: stack.drop(2)
        state.env = env
        state.code = code.drop(1)
        state.dump = dump
      }
      case h:SEL => {
        state.stack = stack.drop(1)
        state.env = env
        state.code = if(stack.head.asInstanceOf[CInteger].i toBool) code(1).asInstanceOf[InstructionBlock].l else code(2).asInstanceOf[InstructionBlock].l
        state.dump = InstructionBlock(code.drop(3)) :: dump
      }
      case h:JOIN => {
        state.stack = stack
        state.env = env
        state.code = dump.head.asInstanceOf[InstructionBlock].l
        state.dump = dump.drop(1)
      }
      case h:LDF => {
        state.stack = Closure(state.code(1).asInstanceOf[InstructionBlock].l, state.env) :: stack
        state.env = env
        state.code = code.drop(2)
        state.dump = dump
      }

      case h:AP => {
        state.stack = List()
        // apply list of values to closure
        state.env = stack(1).asInstanceOf[CList].list :: stack(0).asInstanceOf[Closure].env
        state.code = stack(0).asInstanceOf[Closure].fun
        state.dump = CellValueBlock(stack.drop(2)) :: EnvBlock(env) :: InstructionBlock(code.drop(1)) :: dump
      }

      case h:RTN => {
        state.stack = stack.head :: dump(0).asInstanceOf[CellValueBlock].l
        state.env = dump(1).asInstanceOf[EnvBlock].l
        state.code = dump(2).asInstanceOf[InstructionBlock].l
        state.dump = dump.drop(3)
      }

      case h:DUM => {
        state.stack = stack
        state.env = List() :: env
        state.code = code.drop(1)
        state.dump = dump
      }

      case h:RAP => {
        state.stack = List()
        // apply list of values to closure
        state.env = stack(1).asInstanceOf[CList].list :: stack(0).asInstanceOf[Closure].env.drop(1)
        state.code = stack(0).asInstanceOf[Closure].fun
        state.dump = CellValueBlock(stack.drop(2)) :: EnvBlock(env.drop(1)) :: InstructionBlock(code.drop(1)) :: dump
      }
      case h:WRITEC => {
        state.stack = stack.drop(1)
        state.env = env
        state.code = code.drop(1)
        state.dump = dump
        print(stack.head.asInstanceOf[CInteger].i toChar)
      }
    }
  }

  def applyUnary(v: CellValue, op: UnaryOp): CellValue = {
    op match {
      case Atom() => CInteger(!v.isInstanceOf[CList] toInt)
      case Null() => CInteger(v.isInstanceOf[CNil] || (v.isInstanceOf[CList] && v.asInstanceOf[CList].list.isEmpty) toInt)
      case Car() => v.asInstanceOf[CList].list.head
      case Cdr() => CList(v.asInstanceOf[CList].list.tail)
    }
  }

  def applyBinary(v1: CellValue, v2: CellValue, op: BinaryOp): CellValue = {
    op match {
      case Cons() =>
        val newList = (if(v2.isInstanceOf[CNil]) List()
                       else v2.asInstanceOf[CList].list)
        CList(v1 :: newList)
      case Add() =>  CInteger(v1.asInstanceOf[CInteger].i +  v2.asInstanceOf[CInteger].i)
      case Sub() =>  CInteger(v1.asInstanceOf[CInteger].i -  v2.asInstanceOf[CInteger].i)
      case Mult() => CInteger(v1.asInstanceOf[CInteger].i *  v2.asInstanceOf[CInteger].i)
      case Div() =>  CInteger(v1.asInstanceOf[CInteger].i /  v2.asInstanceOf[CInteger].i)
      case Eq() =>   CInteger(v1.asInstanceOf[CInteger].i == v2.asInstanceOf[CInteger].i toInt)
      case Neq() =>  CInteger(v1.asInstanceOf[CInteger].i != v2.asInstanceOf[CInteger].i toInt)
      case Gt() =>   CInteger(v1.asInstanceOf[CInteger].i >  v2.asInstanceOf[CInteger].i toInt)
      case Lt() =>   CInteger(v1.asInstanceOf[CInteger].i <  v2.asInstanceOf[CInteger].i toInt)
      case Gte() =>  CInteger(v1.asInstanceOf[CInteger].i >= v2.asInstanceOf[CInteger].i toInt)
      case Lte() =>  CInteger(v1.asInstanceOf[CInteger].i <= v2.asInstanceOf[CInteger].i toInt)
    }
  }
}

