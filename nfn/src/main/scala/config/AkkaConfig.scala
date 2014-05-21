package config

import com.typesafe.config.{Config, ConfigFactory}
import scala.concurrent.duration._
import akka.util.Timeout

/**
 * Created by basil on 08/04/14.
 */
object AkkaConfig {


  val timeoutDuration: FiniteDuration = 20 seconds
  implicit val timeout = Timeout( timeoutDuration)

  val configInfo =
    ConfigFactory.parseString("""
      |akka.loglevel=Info
      |akka.debug.lifecycle=on
      |akka.debug.receive=on
      |akka.debug.event-stream=on
      |akka.debug.unhandled=on
      |akka.log-dead-letters=10
      |akka.log-dead-letters-during-shutdown=on
    """.stripMargin
    )

  val configDebug =
    ConfigFactory.parseString("""
                                |akka.loglevel=DEBUG
                                |akka.debug.lifecycle=on
                                |akka.debug.receive=on
                                |akka.debug.event-stream=on
                                |akka.debug.unhandled=on
                                |akka.log-dead-letters=10
                                |akka.log-dead-letters-during-shutdown=on
                              """.stripMargin
    )
}
