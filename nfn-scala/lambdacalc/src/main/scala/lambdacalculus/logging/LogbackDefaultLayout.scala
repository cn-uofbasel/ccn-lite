package lambdacalculus.logging

import ch.qos.logback.core.LayoutBase
import ch.qos.logback.classic.spi.ILoggingEvent
import java.util.Date
import java.text.SimpleDateFormat

class LogbackDefaultLayout extends LayoutBase[ILoggingEvent] {

  override def doLayout(event:ILoggingEvent): String = {
    val elapsedTimeMs = event.getTimeStamp - event.getLoggerContextVO.getBirthTime
    val date = new Date(elapsedTimeMs)
    val formatter = new SimpleDateFormat("ss:SSS")
    val elapsedTime = formatter.format(date)

    val level = event.getLevel

    val loggerName = event.getLoggerName.split("\\.").last

    val msg = event.getFormattedMessage

    val msgStr = s"($elapsedTime) [$level] $loggerName: \n$msg"

    val longestLineOfMsg = msgStr.split("\n").reduceLeft( (s1, s2) =>
      if(s1.size > s2.size) s1 else s2
    ).size

    val msgSep = "-" * (if(longestLineOfMsg <80) longestLineOfMsg else 80)

    s"$msgStr\n$msgSep\n"
  }
}

