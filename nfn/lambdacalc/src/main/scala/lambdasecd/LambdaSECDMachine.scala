package lambdasecd

import scala.collection.immutable.ListMap

class LambdaSECDMachine(debug: Boolean) {

  def apply(expr:Expr):List[Expr] = step(startCfg(expr)).S

  def startCfg(expr: Expr) = Configuration(List(), List(), List(expr), null)

  def step(cfg: Configuration):Configuration = {
    def transformationIsAllowed(cfg:Configuration) = !(cfg.C.isEmpty && cfg.D == null)

    if(debug) println(cfg)

    if(transformationIsAllowed(cfg))
      step(transform(cfg))
    else
      cfg
  }

  def transform(cfg: Configuration): Configuration = {
    val S = cfg.S
    val E = cfg.E
    val C = cfg.C
    val D = cfg.D

    var nextS = S
    var nextE = E
    var nextC = C
    var nextD = D

    var transformNum = -1
    if(C.isEmpty) {
      transformNum = 7
      nextS = if(S.isEmpty) Nil else S.head :: D.S
      nextE = D.E
      nextC = D.C
      nextD = D.D
    } else if(C.head.isInstanceOf[Constant] || C.head.isInstanceOf[PredefinedFunction]) {
      transformNum = 1
      nextS = C.head :: S
      nextC = C.tail
    } else if(C.head.isInstanceOf[Variable]) {
      transformNum = 2
      val variableName = C.head.asInstanceOf[Variable].name
      nextS = if(E.isEmpty) Nil else E.head(variableName) :: S
      nextC = C.tail
    } else if(C.head.isInstanceOf[Apply]) {
      transformNum = 3
      val apply = C.head.asInstanceOf[Apply]
      nextC = apply.rator :: apply.rand :: ApplySymbol() :: C.tail
    } else if(C.head.isInstanceOf[Lambda]) {
      transformNum = 4
      val lambda = C.head.asInstanceOf[Lambda]
      nextS = Closure(lambda.boundVariable, lambda.body, E) :: S
      nextC = C.tail
    } else if(C.head.isInstanceOf[ApplySymbol] && (S.tail.head.isInstanceOf[PredefinedFunction] || S.tail.head.isInstanceOf[UnaryMultFunction]) ) {
      transformNum = 5
      val ExprOfApplication = if(S.tail.head.isInstanceOf[PredefinedFunction])
        S.tail.head.asInstanceOf[PredefinedFunction].apply(S.head)
      else
        S.tail.head.asInstanceOf[UnaryMultFunction].apply(S.head)
      nextS = ExprOfApplication :: S.tail.tail
      nextC = C.tail
    } else if(C.head.isInstanceOf[ApplySymbol]) {
      transformNum = 6
      val closure = S.tail.head.asInstanceOf[Closure]

      nextS = List()
      val currentEnv = if(closure.env.isEmpty) ListMap[String, Expr]() else closure.env.head
      nextE = currentEnv + (closure.boundVariable -> S.head) :: (if(closure.env.isEmpty) List() else closure.env.tail)
      nextC = List(closure.body)
      nextD = Configuration(S.tail.tail, E, C.tail, D)
    } else {
      println("!!!!!!!!!!!!!!!!!!")
      println("Could not evaluate ")
      println("!!!!!!!!!!!!!!!!!!")
      println(cfg)
      System.exit(0)
    }

    val newCfg = Configuration(nextS, nextE, nextC, nextD)

    if(debug) println( "------------")
    if(debug) println(s"transform: $transformNum")
    if(debug) println( "------------")
    if(debug) println(newCfg)
    newCfg
  }
}

