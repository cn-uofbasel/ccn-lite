package secmachine.machine.CallByValue

import secmachine.machine._
import com.typesafe.scalalogging.slf4j.Logging

case class CBVMachine(override val storeIntermediateSteps:Boolean = false) extends Machine with Logging {

  type AbstractConfiguration = CBVConfiguration

  override def startCfg(code: List[Instruction]): CBVConfiguration =  {
    CBVConfiguration(List(), List(), code)
  }

  override def result(cfg: CBVConfiguration): List[Value] = cfg.stack

  override def transform(state: CBVConfiguration): CBVConfiguration = {

    val stack = state.stack
    val env = state.env
    val code = state.code

    var nextStack = stack
    var nextEnv = env
    var nextCode = code

    val instr = state.code.head
    logger.debug(s">>> transform($instr) >>>")
    try {
    instr match {
      // [ s | e | CONST(n).c ] -> [ n.s | e | c ]
      case CONST(const) => {
        val s = stack
        val e = env
        val n = ConstValue(const)
        val c = code.tail

        nextStack = n :: s
        nextEnv = e
        nextCode = c
      }
      // [ s | e | ACCESS(n).c ] -> [ e(n).s | e | c ]
      case ACCESSBYVALUE(n, name) => {
        val s = stack
        val e = env
        val accessedE:Value = if(e.size > n) {
            e(n)
          }else {
            logger.debug(s"name $name cannot be found in the current environment, transforming into final configuration with current state as closure")
            VariableValue(name)
          }

        accessedE match {
          // The accessed name is a list of code, add it to the current code to execute it
          case CodeValue(cl) => {
            val c = code.tail
            nextStack = s
            nextEnv = e
            nextCode = cl ++ c

          }
          //
          case _ => {
            val c = code.tail

            nextStack = accessedE :: s
            nextEnv = e
            nextCode = c
          }
        }
      }
      // [ v.s | e | LET.c ] -> [ s | v.e | c ]
      case LET(cl) => {
        val s = stack
        val e = env
        val c = code.tail
        val v = CodeValue(cl)

        nextStack = s
        nextEnv = v :: e
        nextCode = c
      }
      // [ s | v.e | ENDLET.c ] -> [ s | e | c ]
//      case ENDLET() => {
////        val s = stack
////        val v = env.head
////        val e = env.tail
////        val c = code.tail
//        val s = stack
//        val e = env
//        val c = code.tail
//
//        nextStack = s
//        nextEnv = e
//        nextCode = c
//      }
      // [ s | e | CLOSURE(c').c ] -> [ clo(c', e).s | e | c ]
      case CLOSURE(_, ct) => {
        val s = stack
        val e = env
        val c = code.tail

        nextStack = ClosureValue(ct, e) :: s
        nextEnv = e
        nextCode = c
      }
      // [ v.clo(c', e').s | e | APPLY.c ] -> [ c.e.s | v.e' | c' ]
      case APPLY() => {
        val v = stack.head
        val closure = stack.tail.head.asInstanceOf[ClosureValue]
        val ct = closure.c
        val et = closure.e
        val s = stack.tail.tail
        val e = EnvValue(env)
        val c = CodeValue(code.tail)

        nextStack = c :: e :: s
        nextEnv = v :: et
        nextCode = ct
      }
      // [ v.c'.e'.s | e | RETURN.c ] -> [ v.s | e' | c' ]
      case RETURN() => {
        val v = stack.head
        val ct = stack.tail.head.asInstanceOf[CodeValue].c
        val et = stack.tail.tail.head.asInstanceOf[EnvValue].c
        val s = stack.tail.tail.tail
        val e = env
        val c = code.tail

        nextStack = v :: s
        nextEnv = et
        nextCode = ct
      }
      // TODO: comments
      // TODO: IF and THENELSE are duplicated in both machines
      case IF(test) => {
        nextStack = stack
        nextEnv = env
        nextCode = test ++ code.tail
      }
      case THENELSE(thenn, otherwise) => {
        val thenElseCode = stack.head match {
          case ConstValue(n) => if(n != 0) thenn else otherwise
          case _ => throw new MachineException(s"CBNMachine: top of stack needs to be of ConstValue to check the test case of an if-then-else epxression")
        }

        nextStack = stack.tail
        nextEnv = env
        nextCode = thenElseCode ++ code.tail
      }
      // [ v.s | e | OP(op).c ] -> [ op(v).s | e | c ]
      case op:Unary => {
        val s = stack.tail
        val e = env
        val v = stack.head
        val c = code.tail

        nextStack = op(v) :: s
        nextEnv = e
        nextCode = c
      }

      // [ v1.v2.s | e | OP(op).c ] -> [ op(v1, v2).s | e | c ]
      case op: BINARYOP => {
        val s = stack.tail.tail
        val e = env
        val v1 = stack.head
        val v2 = stack.tail.head
        val c = code.tail

        nextStack = op(v1, v2) :: s
        nextEnv = e
        nextCode = c
      }
    }

    } catch {
      case e: UnsupportedOperationException => throw new MachineException(e.getMessage)
    }
    CBVConfiguration(nextStack, nextEnv, nextCode)
  }

}

