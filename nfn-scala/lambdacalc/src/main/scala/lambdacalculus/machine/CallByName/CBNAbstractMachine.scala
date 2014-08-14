package lambdacalculus.machine.CallByName

import lambdacalculus.machine._

case class CBNAbstractMachine(override val storeIntermediateSteps:Boolean = false, maybeExecutor: Option[CallExecutor] = None) extends AbstractMachine {

  type AbstractConfiguration = CBNConfiguration

  override def startCfg(code: List[Instruction]): CBNConfiguration = {
    CBNConfiguration(List(), List(), code)
  }

  override def result(cfg: CBNConfiguration): List[Value] = cfg.stack

  override def transform(state:CBNConfiguration): CBNConfiguration = {

    val stack = state.stack
    val env = state.env
    val code = state.code

    var nextStack = stack
    var nextEnv = env
    var nextCode = code

    val inst = code.head

    logger.debug(s">>> transform($inst) >>>")
    try {
    inst match {
      // [ s | e | ACCESS(n).c ] -> [ e(n).s | et | ct ]
      // or if function on the lefthand side is unkown:
      // [ s | e | c ] -> [ c[e] |  |  ]
      case ACCESSBYNAME(n, name) => {
        if(n < env.size) {
          val s = stack
          val c = code.tail
          val e = env
          env(n) match {
            case clos: ClosureThunk => {
              logger.debug(s"ClosureThunk")
              val ct = clos.c
              val et = clos.e

              nextStack = s
              nextEnv = et ++ e
              nextCode = ct ++ c
            }
            case codeVal: CodeValue => {
              logger.debug(s"CodeVal")
              val v = codeVal.c

              nextStack = s
              nextEnv = e
              nextCode = v ++ c
            }
            case _ => throw new MachineException(s"ACCESSBYNAME expected a ClosureThunk, found: ${env(n)}")
          }
        } else {
          logger.debug(s"name $name cannot be found in the current environment, transforming into final configuration with current state as closure")
          val c = code
          val e = env
          nextStack = ClosureThunk(c, e) :: Nil
          nextEnv = Nil
          nextCode = Nil
        }
      }
      // [ c'[e'].s | e | GRAB.c ] -> [ s | c'[e'].e | c ]
      // or if function has no value to be applied to:
      // [ s | e | c ] -> [ c[e] |  |  ]
      case GRAB() => {
        if(!stack.isEmpty) {
          val s = stack.tail
          val ctet = stack.head
          val e = env
          val c = code.tail

          nextStack = s
          nextEnv = ctet :: e
          nextCode = c
        } else {
          logger.debug(s"No value found to apply to function ${code.tail.head}, transforming into final configuration with current state as closure")
          val c = code
          val e = env
          nextStack = ClosureThunk(c, e) :: Nil
          nextEnv = Nil
          nextCode = Nil
        }
      }
      // [ s | e | PUSH(c').c] -> [ c'[e].s | e | c ]
      case PUSH(ct) => {
        val s = stack
        val e = env
        val cte = ClosureThunk(ct, e)
        val c = code.tail

        nextStack = cte :: s
        nextEnv = e
        nextCode = c
      }
      // [ s | e | CONST(n).c ] -> [ n.s | e | c ]
      case CONST(const) => {
        val s = stack
        val e = env
        val v = ConstValue(const)
        val c = code.tail

        nextStack = v :: s
        nextEnv = e
        nextCode = c
      }
      // [ v.s | e | OP(op).c ] -> [ op(v).s | e | c ]
      case op: Unary => {
        val s = stack.tail
        val e = env
        val v = stack.head
        val c = code.tail

        nextStack = op(v) :: s
        nextEnv = e
        nextCode = c
      }
      case LET(defName, cl) => {
        val s = stack
        val e = env
        val c = code.tail
        val v = CodeValue(cl, Some(defName))

        nextStack = s
        nextEnv = v :: e
        nextCode = c
      }
//      case ENDLET() => {
//        val s = stack
//        val v = env.head
//        val e = null
//        val c = code.tail
//
//        nextStack = s
//        nextEnv = e
//        nextCode = c
//      }

      case IF(test) => {
        nextStack = stack
        nextEnv = env
        nextCode = test ++ code.tail
      }
      case THENELSE(thenn, otherwise) => {
        val thenElseCode = stack.head match {
          case ConstValue(n, _) => if(n != 0) thenn else otherwise
          case _ => throw new MachineException(s"CBNAbstractMachine: top of stack needs to be of ConstValue to check the test case of an if-then-else epxression")
        }

        nextStack = stack.tail
        nextEnv = env
        nextCode = thenElseCode ++ code.tail
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
      case _ => throw new MachineException(s"CBNAbstractMachine: unkown Instruction: ${code.head}")
    }
    } catch {
      case e: UnsupportedOperationException => throw new MachineException("CBNAbstractMachine" + e.getMessage)
    }
    CBNConfiguration(nextStack, nextEnv, nextCode)
  }
}

