package monitor

import nfn.NodeConfig

/**
 * Created by basil on 17/04/14.
 */
object TestApp extends App {
  val nodeConfs =
    Seq(
      NodeConfig("asdf1", 1, -1, "asdf"),
      NodeConfig("asdf2", 1, -1, "asdf"),
      NodeConfig("asdf3", 1, -1, "asdf"),
      NodeConfig("asdf4", 1, -1, "asdf")
    )

  val nodes = nodeConfs.toSet
  val edges = Set(
    Pair(nodeConfs(0), nodeConfs(1)),
    Pair(nodeConfs(1), nodeConfs(2)),
    Pair(nodeConfs(2), nodeConfs(3)),
    Pair(nodeConfs(3), nodeConfs(0))
  )

  new GraphFrame(nodes, edges)
}
