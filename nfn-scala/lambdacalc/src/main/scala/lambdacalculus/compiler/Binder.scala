package lambdacalculus.compiler

import lambdacalculus.parser.ast._

object Binder {

  def apply(ast: Expr): Expr = bind(ast, List())

  private def bind(expr: Expr, scope: List[List[String]]): Expr = {
    expr match {
      case Application(rator, rand) => Application(bind(rator, scope), bind(rand, scope))
      case Clos(name, body) => {
        val (head, tail) = if(scope.isEmpty) (Nil, Nil) else (scope.head, scope.tail)

        Clos(name, bind(body, (name :: head) :: tail))
      }
      case v @ Variable(name, _) => {
        val accessValue = scope.flatten.view.takeWhile(n => n != name).size
        if(accessValue >= 0)
          Variable(name, accessValue)
        else
          v
      }
      case Let(name, fun, program:Option[Expr]) => {
        val (head, tail) = if(scope.isEmpty) (Nil, Nil) else (scope.head, scope.tail)

        val boundFun = bind(fun, (name :: head) :: tail)
        val boundProgram = program.map(prog => bind(prog, (name :: head) :: tail))
        Let(name, boundFun, boundProgram)
      }
      case UnaryExpr(op, e) => UnaryExpr(op, bind(e, scope))
      case BinaryExpr(op, e1, e2) => BinaryExpr(op, bind(e1, scope), bind(e2, scope))
      case IfElse(test, thenn, otherwise) => {
        IfElse(bind(test, scope), bind(thenn, scope), bind(otherwise, scope))
      }
      case c: Constant => c
      case Call(name, args) => Call(name, args map { bind(_, scope) })
      case _ => throw new Exception(s"Binder has no implementation for expression $expr")
    }
  }
}
