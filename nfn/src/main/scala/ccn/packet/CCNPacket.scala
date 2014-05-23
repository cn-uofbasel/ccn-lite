package ccn.packet

import com.typesafe.scalalogging.slf4j.Logging

sealed trait Packet

/**
 * Marker trait for acknowledgement
 */
trait Ack

object CCNName {
//  def apply(cmps: String *): CCNName = CCNName(cmps.toList)
  def withAddedNFNComponent(ccnName: CCNName) = CCNName(ccnName.cmps ++ Seq("NFN") :_*)
  def withAddedNFNComponent(cmps: Seq[String]) = CCNName(cmps ++ Seq("NFN") :_*)

  def fromString(name: String):Option[CCNName] = {
    if(name.startsWith("/") == false) None
    else {
      Some(CCNName(name.split("/").tail:_*))
    }
  }
}


case class CCNName(cmps: String *) extends Logging {
  def to = toString.replaceAll("/", "_").replaceAll("[^a-zA-Z0-9]", "-")
  override def toString = {
    if(cmps.size == 0) "/"
    else cmps.toList.mkString("/", "/", "")
  }

  def isThunk: Boolean = cmps.size >= 2 && cmps.drop(cmps.size-2).forall(_ != "THUNK") == false

  def isNFN: Boolean = cmps.size >= 1 && cmps.last == "NFN"

  def isCompute: Boolean = (cmps.size >= 1) && cmps.head == "COMPUTE"

  def withoutThunkAndIsThunk: (CCNName, Boolean) = {
    if(cmps.size == 0) this -> false                                  // name '/' is not a thunk
    cmps.last match {
      case "THUNK" =>                                                 // thunk of normal ccn name like /docRepo/doc/1/THUNK
        CCNName(cmps.init:_*) -> true                                 // return /docRepo/doc/1 -> true
      case "NFN" =>                                                   // nfn thunk like /add 1 1/(maybe: THUNK)/NFN
        val cmpsWithoutNFNCmp = cmps.init                             // remove last nfn tag
        if(cmpsWithoutNFNCmp.size == 0) this -> false
        cmpsWithoutNFNCmp.last match {                                // name '/NFN' is not a thunk
          case "THUNK" =>                                             // nfn thunk like /add 1 1/THUNK/NFN
            CCNName(cmpsWithoutNFNCmp.init ++ Seq("NFN"):_*) -> true  // return /add 1 1/NFN
          case _ => this -> false                                     //return /add 1 1/NFN -> false (original name)
        }
      case _ =>                                                       // normal ccn name like /docRepo/doc/1
        this -> false                                                 // return /docRepo/doc/1 -> false (original name)
    }
  }

  def thunkify: CCNName = {
    cmps match {
      case _ if cmps.last == "NFN" =>
        cmps.init match {
          case cmpsWithoutLast if cmpsWithoutLast.last == "THUNK" => this
          case cmpsWithoutLast => CCNName(cmpsWithoutLast ++ Seq("THUNK", "NFN"): _*)
        }
      case _ if cmps.last == "THUNK" => this
      case _ => CCNName(cmps ++ Seq("THUNK"): _*)
    }
  }

  def append(cmpsToAppend:String*):CCNName = CCNName(cmps ++ cmpsToAppend:_*)
  def prepend(cmpsToPrepend:String*):CCNName = CCNName(cmpsToPrepend ++ cmps:_*)
  def append(nameToAppend:CCNName):CCNName = append(nameToAppend.cmps:_*)
  def prepend(nameToPrepend:CCNName):CCNName = prepend(nameToPrepend.cmps:_*)
}

sealed trait CCNPacket extends Packet {
  def name: CCNName
}

object NFNInterest {
  def apply(cmps: String *): Interest = Interest(CCNName(cmps ++ Seq("NFN") :_*))
}

object Interest {
  def apply(cmps: String *): Interest = Interest(CCNName(cmps :_*))
}
case class Interest(name: CCNName) extends CCNPacket {

  def this(cmps: String *) = this(CCNName(cmps:_*))

  def thunkify: Interest = Interest(name.thunkify)


}

object Content {

  def apply(data: Array[Byte], cmps: String *): Content =
    Content(CCNName(cmps :_*), data)

  def thunkForName(name: CCNName, executionTimeEstimated: Option[Int]) = {
    val thunkyfiedName = name.thunkify
    val thunkContentData = executionTimeEstimated.fold("")(_.toString)
    Content(thunkyfiedName, thunkContentData.getBytes)
  }
  def thunkForInterest(interest: Interest, executionTimeEstimate: Option[Int]): Content = {
    thunkForName(interest.name, executionTimeEstimate)
  }
}

case class Content(name: CCNName, data: Array[Byte]) extends CCNPacket {

//  def this(data: Array[Byte], cmps: String *) = this(CCNName(cmps.toList), data)
  def possiblyShortenedDataString: String = {
    val dataString = new String(data)
    if(dataString.length > 50) dataString.take(50) + "..." else dataString
  }
  override def toString = s"Content('$name' => $possiblyShortenedDataString)"
}

case class AddToCache() extends Packet with Ack
