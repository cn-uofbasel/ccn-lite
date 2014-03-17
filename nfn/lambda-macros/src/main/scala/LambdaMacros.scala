import scala.reflect.macros.Context
import scala.language.experimental.macros

import secmachine.parser.ast._
import secmachine.parser.ast.{Constant => LConstant}
import secmachine.parser.ast.{Expr => LExpr}

object LambdaMacros {
  def hello(): Unit = macro hello_impl

  def hello_impl(c: Context)(): c.Expr[Unit] = {
    import c.universe._
    reify { println("Hello World!") }
  }

  def printparam(param: Any): Unit = macro printparam_impl

  def printparam_impl(c: Context)(param: c.Expr[Any]): c.Expr[Unit] = {
    import c.universe._
    reify { println(param.splice)}
  }

  def debug1(param: Any): Unit = macro debug1_impl

  def debug1_impl(c: Context)(param: c.Expr[Any]): c.Expr[Unit] = {
    import c.universe._

    val paramRep = show(param.tree)
    val paramRepTree = Literal(Constant(paramRep))
    val paramRepExpr = c.Expr[String](paramRepTree)
    reify { println(paramRepExpr.splice + " = " + param.splice)}
  }

  def debug(params: Any*): Unit = macro debug_impl

  def debug_impl(c: Context)(params: c.Expr[Any]*): c.Expr[Unit] = {
    import c.universe._

    val trees = params.map { param =>
      param.tree match {
        case Literal(Constant(const)) => {
          val reified = reify { print(param.splice) }
          reified.tree
        }
        case _ => {
          val paramRep = show(param.tree)
          val paramRepTree = Literal(Constant(paramRep))
          val paramRepExpr = c.Expr[String](paramRepTree)
          val reified = reify { print(paramRepExpr.splice + " = " + param.splice)}
          reified.tree
        }
      }
    }

    val seperators = (1 to trees.size-1).map(_ => reify { print(", ")}.tree) :+ (reify {println()}).tree
    val treesWithSeperators = trees.zip(seperators).flatMap(p => List(p._1, p._2))

    c.Expr[Unit](Block(treesWithSeperators.toList, Literal(Constant())))
  }

  def nop(param: Any): Unit = macro nop_impl

  def nop_impl(c: Context)(param: c.Expr[Any]): c.Expr[Unit] = {
    import c.universe._

    reify { }
  }

  def lambda(param: Any): String = macro lambda_impl

  def lambda_impl(c: Context)
                 (param: c.Expr[Any]) : c.Expr[String] = {

//  def lambda[T](param: T): String = macro lambda_impl[T]
//
//  def lambda_impl[T: c.WeakTypeTag](c: Context)
//                 (param: c.Expr[T]) : c.Expr[String] = {
    import c.universe._
//    def toL(str: String): String = toLambda(c.Expr[Any](q"$str"))
    //    def toL(qq: QuasiquoteCompat): String = toLambda(c.Expr[Any](q"$qq"))


    // Transforms a select chain into a single qualified name
    // The typer of the compiler always fully qualifies names
//    def selectChain

//    def treeToPrettyString(expr: Tree): String = {
//
//      val sb = new StringBuilder
//
//      def parseTreeLevelToBuffer(treeExpr: Tree, level: Int): Unit = {
//
//        def notImplemented(exprName: String) = (s"!$exprName!", s"/$exprName")
//
//        val (exprString, exprClosingString) = treeExpr match {
//          case ValDef(mods: Modifiers, name: TermName, tptTree:Tree, rhs:Tree) => {
//            notImplemented("ValDef")
//          }
//          case DefDef(mods: Modifiers, name: Name, tparams: List[TypeDef], vparamss: List[List[ValDef]], tptTree: Tree, rhs: Tree) => {
//            notImplemented("DefDef")
//          }
//          case TypeDef(mods: Modifiers, name: TypeName, tparams: List[TypeDef], rhs: Tree) => {
//            notImplemented("TypeDef")
//          }
//          case PackageDef(pid: RefTree, stats: List[Tree]) => {
//            notImplemented("PackageDef")
//          }
//          case ModuleDef(mods: Modifiers, name: TermName, impl: Template) => {
//            notImplemented("ModuleDef")
//          }
//          case ClassDef(mods: Modifiers, name: TypeName, tparams: List[TypeDef], impl: Template) => {
//            notImplemented("ClassDef")
//          }
//          case Bind(name: Name, body: Tree) => {
//            notImplemented("Bind")
//          }
//          case LabelDef(name: TermName, params: List[Ident], rhs: Tree) => {
//            notImplemented("LabelDef")
//          }
//          case Select(qualifier: Tree, name: Name) => {
//            (s"Select($name)", s"/Select")
//          }
//          case Ident(name: Name) => {
//            notImplemented("Ident")
//          }
//          case Block(stats: List[Tree], blockExpr: Tree) => {
//            ("{", "}")
//          }
//          case Assign(lhs: Tree, rhs: Tree) => {
//            notImplemented("Assign")
//          }
//          case Alternative(trees: List[Tree]) => {
//            notImplemented("Alternative")
//          }
//          case UnApply(fun: Tree, args: Tree) => {
//            notImplemented("UnApply")
//          }
//          case Typed(expr: Tree, tpt: Tree) => {
//            notImplemented("Typed")
//          }
//          case Try(block: Tree, catches: List[CaseDef], finalizer: Tree) => {
//            notImplemented("Try")
//          }
//          case Throw(expr: Tree) => {
//            notImplemented("Throw")
//          }
//          case Star(elem: Tree) => {
//            notImplemented("Star")
//          }
//          case New(tpt: Tree) => {
//            notImplemented("New")
//          }
//          case Match(selector: Tree, cases: List[CaseDef]) => {
//            notImplemented("Match")
//          }
//          case Literal(value: Constant) => {
//            notImplemented("Literal")
//          }
//          case If(cond: Tree, thenp: Tree, elsep: Tree) => {
//            notImplemented("If")
//          }
//          case Super(qual: Tree, mix: TypeName) => {
//            notImplemented("Super")
//          }
//          case Return(expr: Tree) => {
//            notImplemented("Return")
//          }
//          case Function(vparams: List[ValDef], body: Tree) => {
//            notImplemented("Function")
//          }
//          case This(qual: TypeName) => {
//            notImplemented("This")
//          }
//          case Apply(fun: Tree, args: List[Tree]) => {
//            notImplemented("Apply")
//          }
//          case TypeApply(fun: Tree, args) => {
//            notImplemented("TypeApply")
//          }
//          case SelectFromTypeTree(qualifier: Tree, name: TypeName) => {
//            notImplemented("SelectFromTypeTree")
//          }
//          case t: TypTree => {
//            notImplemented("TypTree and child types")
//          }
//          case Template(parents: List[Tree], self: ValDef, body: List[Tree]) => {
//            notImplemented("Template")
//          }
//          case Import(expr: Tree, selectors: List[ImportSelector]) => {
//            notImplemented("Import")
//          }
//        }
//        sb.append("\t" * level + exprString + "\n")
//        parseTreeLevelToBuffer(expr.children.head, level + 1)
////        expr.children.foreach(parseTreeLevelToBuffer(_, level + 1))//foparsetreeleveltobuffer(expr.children)
//        sb.append("\t" * level + exprClosingString + "\n")
//      }
//
//      sb.append("\n===|TREE|===\n")
//      parseTreeLevelToBuffer(expr, 0)
////      expr match {
////        case Expr(e) =>  parseTreeLevelToBuffer(e, 0)
////        case _ => throw new Exception("Tree pretty print: Top level object is not of type Expr")
////      }
//      sb.append("===/TREE/===\n")
//      sb.toString
//    }

    def toLambda(treeExpr: Tree): LExpr = {

      def matchInfo(info: String) = println(s"Matched [ ${treeExpr} ] with [ $info ]")

      def notImplemented(msg: String) = throw new Exception(s"Macro expansion for $msg not implmented ($treeExpr)")

      treeExpr match {
        // Defs (DefTree's)
        // ====
        // define or introduce new entities, each has a name and introduces a symbol
        // mods: override, abstract, final, lazy, private, protected (SYNTHETIC means it is compiler generated)

        // A Symbol is are related to a specific tree instance, but provide a lot more and context specific information
        // about the program. Symbols are also connected to each other. This enables to retrieve information about
        // class hirachries and similar information.


        case ValDef(mods: Modifiers, name: TermName, tptTree:Tree, rhs:Tree) => {
          matchInfo("ValDef")
          Let(name.toString, toLambda(rhs), None)
        }
        case DefDef(mods: Modifiers, name: Name, tparams: List[TypeDef], vparamss: List[List[ValDef]], tptTree: Tree, rhs: Tree) => {
          matchInfo("DefDef")
          val nParams = tparams.length
          val codeExpr = toLambda(rhs)
          val expr = tparams.foldLeft(codeExpr)((curExpr, param) => Clos(param.name.toString, curExpr))
          //          val typeNames = params.foldLeft("")((curName, param) => s"$curName/${param.tpe.typeSymbol.name}")
          val funName = s"$name"
          Let(funName, expr, None)
        }
        case TypeDef(mods: Modifiers, name: TypeName, tparams: List[TypeDef], rhs: Tree) => {
          notImplemented("TypeDef")
        }
        case PackageDef(pid: RefTree, stats: List[Tree]) => {
          notImplemented("PackageDef")
        }
        case ModuleDef(mods: Modifiers, name: TermName, impl: Template) => {
          notImplemented("ModuleDef")
        }
        case ClassDef(mods: Modifiers, name: TypeName, tparams: List[TypeDef], impl: Template) => {
          notImplemented("ClassDef")
        }
        case Bind(name: Name, body: Tree) => {
          notImplemented("Bind")
        }
        // Represents while and do .. while loops
        // Name holds the name of the label
        case LabelDef(name: TermName, params: List[Ident], rhs: Tree) => {
          notImplemented("LabelDef")
        }
        // Refs (RefTree): References to DefTrees introduces entities, also have a name and their symbol
        // is the same as the corresponding DefTree (can be composed by ==)
        case Select(qualifier: Tree, name: Name) => {
          notImplemented("Select")
        }
        case Ident(name: Name) => {
          matchInfo("Ident")
          Variable(name.toString)
        }
        // Terms (TermTree)
        // =====
        case Block(stats: List[Tree], blockExpr: Tree) => {
          matchInfo("Block")
          val lambdaExprs: List[LExpr] = stats.map(expr => toLambda(expr)) ++ List(toLambda(blockExpr))

          lambdaExprs.reverse.reduce((expr, let) =>
            let match {
              case Let(name, body, _) => Let(name, body, Some(expr))
              case _ => throw new Exception("When merging let expressions, one expression was not of type LET")
            })
        }
        case Assign(lhs: Tree, rhs: Tree) => {
          notImplemented("Assign")
        }
        //        case a: ArrayValue => {
        //
        //        }
        case Alternative(trees: List[Tree]) => {
          notImplemented("Alternative")
        }
        case UnApply(fun: Tree, args: Tree) => {
          notImplemented("UnApply")
        }
        case Typed(expr: Tree, tpt: Tree) => {
          notImplemented("Typed")
        }
        case Try(block: Tree, catches: List[CaseDef], finalizer: Tree) => {
          notImplemented("Try")
        }
        case Throw(expr: Tree) => {
          notImplemented("Throw")
        }
        case Star(elem: Tree) => {
          notImplemented("Star")
        }
        case New(tpt: Tree) => {
          notImplemented("New")
        }
        case Match(selector: Tree, cases: List[CaseDef]) => {
          notImplemented("Match")
        }
        case Literal(value: Constant) => {
          matchInfo(s"Literal($value)")
          value match {
            case Constant(n: Int) => LConstant(n)
            case Constant(()) => NopExpr()
            case Constant(s: String) => Variable(s)
            case Constant(n) => throw new Exception(s"Constant of type $n not implemented")
            case _ => throw new Exception(s"Constant with unkown type: $treeExpr")
          }
        }
        case If(cond: Tree, thenp: Tree, elsep: Tree) => {
          matchInfo("If")
          IfElse(toLambda(cond), toLambda(thenp), toLambda(elsep))
        }
        case Super(qual: Tree, mix: TypeName) => {
          notImplemented("Super")
        }
        case Return(expr: Tree) => {
          notImplemented("Return")
        }
        case Function(vparams: List[ValDef], body: Tree) => {
          notImplemented("Function")
        }
        case This(qual: TypeName) => {
          notImplemented("This")
        }
        case a @ Apply(TypeApply(s @ Select(qualifier: Tree, _), _), args:List[Tree]) => {
          qualifier match {
            case Select(_, qualifiedName) => qualifiedName.toString match {
              case "List" => {
                // TODO: a list was instantiated, parse the arguments and create a lambda list
                val argsExpr = args map { toLambda }
                NopExpr()
              }
              case _ => throw new Exception(s"Apply(TypeApply(Select(...: type of type apply not implemented: $qualifiedName")
            }
            case _ => throw new Exception(s"TypeApply(Select...: qualifier is not of type Select but $qualifier")
          }
        }

        // Select apply
        // Matches both numeric calculations like 1+2 ( 1.+(2) ) and
        // method calls on objects like NFNServiceLibrary.method(...)
        case a @ Apply(Select(qualifier: Tree, fun: Name), args: List[Tree]) => {
          //          implicit def binaryOpToExpr(binOp: BinaryOp.BinaryOp): Expr = BinaryExpr(binOp)
          assert(args.size == 1, s"Apply(Select, _): only implemented for args size of 1 and not ${args.size}")
//          matchInfo(s"Apply(Select($qualifier, $fun), $args)")
          val arg = args.head
          qualifier.toString match {
            case "NFNServiceLibrary" => {
              matchInfo(s"Apply(Select(NFNServiceLibrary, .$fun), $arg)")
              val argTpe = arg.tpe.typeSymbol.name
              val retTpe = a.tpe.typeSymbol.name
              val funName: String = s"$fun"
              val funValue = toLambda(arg)
              val body = Application(Variable(s"/$funName/$argTpe/r$retTpe"), Variable("v"))
              val clos = Clos("v", body)
              Application(clos, funValue)
            }
            case "NFNName" => {
              matchInfo(s"Apply(Select(NFNName<$arg>")
              toLambda(arg)
            }
            case _ => {
              matchInfo(s"Apply(Select($qualifier, $fun), $arg")
              val op = fun.toString match {
                case "$plus" => BinaryOp.Add
                case "$times" => BinaryOp.Mult
                case "$minus" => BinaryOp.Sub
                case "$greater" => BinaryOp.Gt
                case "$less" => BinaryOp.Lt
                case "$less$eq" => BinaryOp.Lte
                case "$greater$eq" => BinaryOp.Gte
                case "$eq$eq" => BinaryOp.Eq
                case "$bang$eq" => BinaryOp.Neq
                case _ => throw new Exception(s"No match for operator $fun")
              }

              val (arg1, arg2) = (toLambda(qualifier), toLambda(arg))
              BinaryExpr(op, arg1, arg2)
            }
          }
        }
        //GenericApply
        case Apply(fun: Tree, args: List[Tree]) => {
          notImplemented("Apply")
          // TODO is this dead code?
          matchInfo(s"Apply (fun: <$fun> args: <$args>)")
          val funExpr = toLambda(fun)
          val argExprs = args map {toLambda}
          NopExpr()
        }
        case TypeApply(fun: Tree, args) => {
          notImplemented("TypeApply")
        }
        case SelectFromTypeTree(qualifier: Tree, name: TypeName) => {
          notImplemented("SelectFromTypeTree")
        }
        case t: TypTree => {
          notImplemented("TypTree and child types")
        }
        case Template(parents: List[Tree], self: ValDef, body: List[Tree]) => {
          notImplemented("Template")
        }
        case Import(expr: Tree, selectors: List[ImportSelector]) => {
          notImplemented("Import")
        }

//        case q"($x: $t) => $body" =>  {
//          matchInfo("('x: 't) => 'body")
//          Application(Variable(s"$x"), body)
//        }
        case _ =>  throw new Exception(s"No match: $treeExpr")//LLiteral(s"$expr")
      }
    }

    println("=" * 80)

    printRaw_impl(c)(param)

    val res: String =
      LambdaPrettyPrinter(toLambda(q"$param"))
//      q"$param" match {
//        case _ => throw new Exception("Can only lambdafy blocks!")
//      }

//      q"$param" match {
//        // lambda abstraction
//        case q"($sym1 => $rhs + $lhs)" =>  s"\\${sym1}\n$lhs +++ $rhs"
//
//        // application
//        case q"$lhs.$op($rhs)" => s"$lhs.$op($rhs)"
//
//        // constant / name
//        case Expr(Literal(Constant(v)))/*q"$num"*/ => {
//          v.toString.forall(Character.isDigit) match {
//            case true => s"$v"
//            case false => s"'$v'"
//          }
//        }
//        case _ => ""
//      }
    if(res == "") println(s"NO MATCH FOR: $param")
    else println(s"lambda<$res>")

    println("=" * 80)

//    reify { param.splice }
//    reify { new String(s"$res")}
      c.Expr[String](q"$res")
  }

  def printRaw(param: Any): Unit = macro printRaw_impl

  def printRaw_impl(c: Context)(param: c.Expr[Any]): c.Expr[Unit] = {
    import c.universe._

    println(s"RAW:\n$param\n===>\n${showRaw(param)}\n")

    reify { () }
  }
}

//// matches a Tree
//treeExpr match {
//  case ValDef(mods: Modifiers, name: TermName, tptTree:Tree, rhs:Tree) => {
//    notImplemented("ValDef")
//  }
//  case DefDef(mods: Modifiers, name: Name, tparams: List[TypeDef], vparamss: List[List[ValDef]], tptTree: Tree, rhs: Tree) => {
//    notImplemented("DefDef")
//  }
//  case TypeDef(mods: Modifiers, name: TypeName, tparams: List[TypeDef], rhs: Tree) => {
//    notImplemented("TypeDef")
//  }
//  case PackageDef(pid: RefTree, stats: List[Tree]) => {
//    notImplemented("PackageDef")
//  }
//  case ModuleDef(mods: Modifiers, name: TermName, impl: Template) => {
//    notImplemented("ModuleDef")
//  }
//  case ClassDef(mods: Modifiers, name: TypeName, tparams: List[TypeDef], impl: Template) => {
//    notImplemented("ClassDef")
//  }
//  case Bind(name: Name, body: Tree) => {
//    notImplemented("Bind")
//  }
//  case LabelDef(name: TermName, params: List[Ident], rhs: Tree) => {
//    notImplemented("LabelDef")
//  }
//  case Select(qualifier: Tree, name: Name) => {
//    s"Select($name)"
//  }
//  case Ident(name: Name) => {
//    notImplemented("Ident")
//  }
//  case Block(stats: List[Tree], blockExpr: Tree) => {
//    notImplemented("Block")
//  }
//  case Assign(lhs: Tree, rhs: Tree) => {
//    notImplemented("Assign")
//  }
//  case Alternative(trees: List[Tree]) => {
//    notImplemented("Alternative")
//  }
//  case UnApply(fun: Tree, args: Tree) => {
//    notImplemented("UnApply")
//  }
//  case Typed(expr: Tree, tpt: Tree) => {
//    notImplemented("Typed")
//  }
//  case Try(block: Tree, catches: List[CaseDef], finalizer: Tree) => {
//    notImplemented("Try")
//  }
//  case Throw(expr: Tree) => {
//    notImplemented("Throw")
//  }
//  case Star(elem: Tree) => {
//    notImplemented("Star")
//  }
//  case New(tpt: Tree) => {
//    notImplemented("New")
//  }
//  case Match(selector: Tree, cases: List[CaseDef]) => {
//    notImplemented("Match")
//  }
//  case Literal(value: Constant) => {
//    notImplemented("Literal")
//  }
//  case If(cond: Tree, thenp: Tree, elsep: Tree) => {
//    notImplemented("If")
//  }
//  case Super(qual: Tree, mix: TypeName) => {
//    notImplemented("Super")
//  }
//  case Return(expr: Tree) => {
//    notImplemented("Return")
//  }
//  case Function(vparams: List[ValDef], body: Tree) => {
//    notImplemented("Function")
//  }
//  case This(qual: TypeName) => {
//    notImplemented("This")
//  }
//  case Apply(fun: Tree, args: List[Tree]) => {
//    notImplemented("Apply")
//  }
//  case TypeApply(fun: Tree, args) => {
//    notImplemented("TypeApply")
//  }
//  case SelectFromTypeTree(qualifier: Tree, name: TypeName) => {
//    notImplemented("SelectFromTypeTree")
//  }
//  case t: TypTree => {
//    notImplemented("TypTree and child types")
//  }
//  case Template(parents: List[Tree], self: ValDef, body: List[Tree]) => {
//    notImplemented("Template")
//  }
//  case Import(expr: Tree, selectors: List[ImportSelector]) => {
//    notImplemented("Import")
//  }
//}

