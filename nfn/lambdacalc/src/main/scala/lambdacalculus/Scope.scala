package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 14:15
 * To change this template use File | Settings | File Templates.
 */

object Scope {
  var id = 0
  def nextId = { val i = id; id += 1; i}
  val TOP = new Scope(None, Set())
}

class Scope(val parent: Option[Scope], val boundNames: Set[String]) {
  val id = Scope.nextId

  def closestBinding(name: String): Option[Scope] =
    if (boundNames contains name)
      Some(this)
    else
    // Option is a Seq with 0 or 1 elements, flatMap will eliminate all None elements and directly
    // leave all the values of the Some elements
      parent flatMap (_ closestBinding name)
}


