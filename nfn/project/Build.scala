import sbt._
import Keys._

object BuildSettings {
  val paradiseVersion = "2.0.0-M3"
  val buildSettings = Defaults.defaultSettings ++ Seq (
    version       := "0.1-SNAPSHOT",
    scalaVersion  := "2.10.3",
    scalacOptions ++= Seq("-unchecked", "-deprecation", "-encoding", "UTF-8"),
    resolvers += Resolver.sonatypeRepo("snapshots"),
    resolvers += Resolver.sonatypeRepo("releases"),
    resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
    addCompilerPlugin("org.scalamacros" % "paradise" % paradiseVersion cross CrossVersion.full)
  )

  // important to use ~= so that any other initializations aren't dropped
  //   the _ discards the meaningless () value previously assigned to 'initialize'

}

object MainBuild extends Build {

  import BuildSettings._


  initialize ~= { _ =>
    println("init java.library.path = ./ccnliteinterface/src/main/c/ccn-lite-bridge")
    System.setProperty("java.library.path", "./ccnliteinterface/src/main/c/ccn-lite-bridge")
  }

  lazy val nfn: Project = Project(
    "nfn",
    file("."),
    settings = buildSettings ++ Seq(
      mainClass := Some("Main"),
      libraryDependencies ++= Seq(
        "com.typesafe.akka" %% "akka-actor" % "2.2.1",
        "com.typesafe.akka" %% "akka-testkit" % "2.2.1",
        "org.scalatest" % "scalatest_2.10" % "2.0" % "test",
        "ch.qos.logback" % "logback-classic" % "1.0.3",
        "com.typesafe" %% "scalalogging-slf4j" % "1.0.1",
        "org.slf4j" % "slf4j-api" % "1.7.5",
        "com.assembla.scala-incubator" % "graph-core_2.10" % "1.8.0",
        "org.tinyjee.jgraphx" % "jgraphx" % "2.3.0.5",
        "jgraph" % "jgraph" % "5.13.0.0",
         "org.scala-lang" %% "scala-pickling" % "0.8.0-SNAPSHOT"
  )
    )
  ).dependsOn(lambdaMacros, ccnliteinterface, lambdaCalculus)
  //aggregate(nfn, lambdaCalculus, lambdaMacros , testservice, ccnliteinterface)

  lazy val lambdaCalculus: Project = Project(
    "lambdacalc",
    file("lambdacalc"),
    settings = buildSettings ++ Seq (
      libraryDependencies ++= Seq(
        "org.scalatest" % "scalatest_2.10" % "2.0" % "test",
        "ch.qos.logback" % "logback-classic" % "1.0.3",
        "com.typesafe" %% "scalalogging-slf4j" % "1.0.1",
        "org.slf4j" % "slf4j-api" % "1.7.5"
      ),
      resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
      mainClass in (Compile, run) := Some("lambdacalculus.LambdaCalculus")
    )
  )

  lazy val lambdaMacros: Project = Project(
    "lambda-macros",
    file("lambda-macros"),
    settings = buildSettings ++ Seq(
      libraryDependencies <+= (scalaVersion)("org.scala-lang" % "scala-compiler" % _),
      libraryDependencies ++= (
        if (scalaVersion.value.startsWith("2.10")) List("org.scalamacros" % "quasiquotes"  % paradiseVersion cross CrossVersion.full)
        else Nil
      )
    )
  ).dependsOn(lambdaCalculus)


  lazy val testservice: Project = Project(
    "testservice",
    file("testservice"),
    settings = buildSettings
  ).dependsOn(nfn)

  lazy val ccnliteinterface: Project = Project(
    "ccnliteinterface",
    file("ccnliteinterface"),
    settings = buildSettings ++ Seq(
      resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
      fork := true,
      javaOptions ++= Seq("-Djava.library.path=./ccnliteinterface/src/main/c/ccn-lite-bridge"),
      libraryDependencies ++= Seq(
        "org.scalatest" % "scalatest_2.10" % "2.0" % "test",
        "ch.qos.logback" % "logback-classic" % "1.0.3",
        "com.typesafe" %% "scalalogging-slf4j" % "1.0.1",
        "org.slf4j" % "slf4j-api" % "1.7.5"
      )
    )
  )
}

