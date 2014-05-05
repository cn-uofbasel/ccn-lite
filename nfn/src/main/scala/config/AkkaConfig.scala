package config

import com.typesafe.config.{Config, ConfigFactory}

/**
 * Created by basil on 08/04/14.
 */
object AkkaConfig {

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
