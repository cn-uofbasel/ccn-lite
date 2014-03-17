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
}

object MainBuild extends Build {
  import BuildSettings._

  lazy val root: Project = Project(
    "root",
    file("."),
    settings = buildSettings ++ Seq(
      run <<= run in Compile in ccnliteinterface
    )
  ) aggregate(lambdaCalculus, lambdaMacros, lambdaCalculusToMacros, testservice, ccnliteinterface)



  lazy val lambdaCalculus: Project = Project(
    "lambdacalc",
    file("lambdacalc"),
    settings = buildSettings ++ Seq (
      libraryDependencies ++= Seq(
//        "org.scala-lang" % "scala-compiler" % "2.10.3",
        "org.scalatest" % "scalatest_2.10" % "2.0" % "test",
        "ch.qos.logback" % "logback-classic" % "1.0.3",
        "com.typesafe" %% "scalalogging-slf4j" % "1.0.1",
        "org.slf4j" % "slf4j-api" % "1.7.5"
      ),
      resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
      mainClass in (Compile, run) := Some("secmachine.LambdaCalculus")
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


  lazy val lambdaCalculusToMacros: Project = Project(
    "lambda-to-macros",
    file("lambda-to-macros"),
    settings = buildSettings ++ Seq(
      mainClass := Some("Main"),
      libraryDependencies ++= Seq(
        "org.scalatest" % "scalatest_2.10" % "2.0" % "test"
      )
    )
  ).dependsOn(lambdaMacros, ccnliteinterface)

  lazy val ccn: Project = Project(
    "ccn",
    file("ccn"),
    settings = buildSettings ++ Seq(
      mainClass := Some("Main"),
      resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
      libraryDependencies ++= Seq(
        "com.typesafe.akka" %% "akka-actor" % "2.2.1"
      )
    )
  )

  lazy val testservice: Project = Project(
    "testservice",
    file("testservice"),
    settings = buildSettings
  ).dependsOn(lambdaCalculusToMacros)

  lazy val ccnliteinterface: Project = Project(
    "ccnliteinterface",
    file("ccnliteinterface"),
    settings = buildSettings ++ Seq(
      resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
      fork := true,
      javaOptions ++= Seq("-Djava.library.path=/Users/basil/Dropbox/uni/master_thesis/code/ccnliteinterface/src/main/c/ccn-lite-bridge"),
      libraryDependencies ++= Seq(
        "com.typesafe.akka" %% "akka-actor" % "2.2.1",
        "org.scalatest" % "scalatest_2.10" % "2.0" % "test"
      )
    )
  )
}

