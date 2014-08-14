package ccn.ccnlite

class NFList[A, B](l: List[A]) {
  def nfnMapReduce[B](mapClosure: A => B, reduceClosure: (B,B) => B ): B = {
    l.map(mapClosure).reduce(reduceClosure)
  }
}

object NFList {
  def apply[String, B](nfs: String*): NFList[String, B] = {
    new NFList(nfs toList)
  }
}

object Test extends App {

  val docs = NFList("/doc/1", "/doc/2", "/doc/3")

  val mapWc = (doc: String) => doc.split("/").last.toInt
  val reduceWc = (l: Int, r: Int) => l + r

  val res = docs.nfnMapReduce[Int](mapWc, reduceWc)
  println (res)

}
