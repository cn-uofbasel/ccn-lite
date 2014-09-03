import java.io._

import sbt.File
import sbt._
import Keys._
import sbtassembly.Plugin.AssemblyKeys._
import sbtassembly.Plugin.assemblySettings

object BuildSettings {
  val paradiseVersion = "2.0.0-M3"
  val buildSettings = Defaults.defaultSettings ++ assemblySettings ++ Seq (
    version       := "0.1-SNAPSHOT",
    scalaVersion  := "2.10.3",
    scalacOptions ++= Seq("-unchecked", "-deprecation", "-encoding", "UTF-8"),
    resolvers += Resolver.sonatypeRepo("snapshots"),
    resolvers += Resolver.sonatypeRepo("releases"),
    resolvers += "Typesafe Repository" at "http://repo.typesafe.com/typesafe/releases/",
    addCompilerPlugin("org.scalamacros" % "paradise" % paradiseVersion cross CrossVersion.full),
    MainBuild.compileJNI,
    test in assembly := {}
  )
}

object MainBuild extends Build {

  import BuildSettings._

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
        "com.typesafe" % "config" % "1.2.1",
        "org.slf4j" % "slf4j-api" % "1.7.5",
        "net.liftweb" %% "lift-json" % "2.5.1",
        "org.apache.bcel" % "bcel" % "5.2"
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

  lazy val nfnScalaExperiments: Project = Project(
    "nfn-scala-experiments",
    file("nfn-scala-experiments"),
    settings = buildSettings
  ).dependsOn(nfn)

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


  val compileJNITask = TaskKey[Unit]("compileJniNativelib")
  val compileJNI = compileJNITask := {
    val processBuilder = new java.lang.ProcessBuilder("./make.sh")
    processBuilder.directory(new File("./ccnliteinterface/src/main/c/ccn-lite-bridge"))
    val process = processBuilder.start()
    val processOutputReaderPrinter = new InputStreamToStdOut(process.getInputStream)
    val t = new Thread(processOutputReaderPrinter).start()
    process.waitFor()
    println(s"Compile JNI Process finished with return value ${process.exitValue()}")
    process.destroy()
  }
}

class InputStreamToStdOut(is: InputStream) extends Runnable {
  override def run(): Unit = {
    val reader = new BufferedReader(new InputStreamReader(is))
    var line = reader.readLine
    while(line != null) {
      println(reader.readLine())
      line = reader.readLine
    }
  }
}

