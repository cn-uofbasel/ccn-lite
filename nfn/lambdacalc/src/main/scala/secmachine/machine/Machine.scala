package secmachine.machine

import com.typesafe.scalalogging.slf4j.Logging


trait Configuration {
  def isTransformable: Boolean
}

case class MachineException(msg: String) extends Exception(msg)

abstract class Machine(val storeIntermediateSteps:Boolean = false) extends Logging {

  type AbstractConfiguration <: Configuration

  var _intermediateConfigurations: List[AbstractConfiguration] = List()

//  def applyWithCommands(code:List[Instruction], cmds: List[CommandInstruction]):List[Value] = {
//    info(s"Executing: $code (lib: $cmds")
//    val cmdsCode = cmds.map(cmdsInstr => cmdsInstr.instructions).flatten
//    result(step(startCfg(cmdsCode ++ code)))
//  }

  def apply(code:List[Instruction]):List[Value] = {
    logger.info(s"Executing code: $code")
    result(step(startCfg(code)))
  }

  def startCfg(code: List[Instruction]): AbstractConfiguration

  def result(cfg: AbstractConfiguration): List[Value]

  def transform(state:AbstractConfiguration): AbstractConfiguration

  def step(cfg: AbstractConfiguration):AbstractConfiguration = {
    logger.debug(cfg.toString)
    if(storeIntermediateSteps) _intermediateConfigurations ::= cfg
    if(cfg.isTransformable) {
      step(transform(cfg))
    } else {
      cfg
    }
  }

  def printIntermediateSteps() =
    if(storeIntermediateSteps) {
      println(_intermediateConfigurations.reverse.mkString("\n===>\n"))
    } else {
      logger.error("Can only print intermediate steps if storeIntermediateSteps is set to true")
    }


  def intermediateConfigurations:Option[List[Configuration]] = if(storeIntermediateSteps) Some(_intermediateConfigurations) else None

}

