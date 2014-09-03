package ccnliteinterface

object CCNLiteInterfaceType {
  def fromName(possibleName: String): Option[CCNLiteInterfaceType] = {
    possibleName match {
      case "jni" => Some(CCNLiteJniInterface())
      case "cli" => Some(CCNLiteCliInterface())
      case _ => None
    }
  }
}
trait CCNLiteInterfaceType
case class CCNLiteJniInterface() extends CCNLiteInterfaceType
case class CCNLiteCliInterface() extends CCNLiteInterfaceType

