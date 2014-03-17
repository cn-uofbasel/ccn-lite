package lambdacalculus

/**
 * Created with IntelliJ IDEA.
 * User: basil
 * Date: 22/11/13
 * Time: 14:05
 * To change this template use File | Settings | File Templates.
 */
object Library {
  val source =
    """
    id = λx.x;

    bool   = λb. b T F;
    number = λn. n S Z;

    zero  = λs. λz. z;
    one   = λs. λz. s z;
    two   = λs. λz. s (s z);
    three = λs. λz. s (s (s z));

    true   = λt. λf. t;
    false  = λt. λf. f;
    if     = λc. λt. λe. c t e;
    or     = λa. λb. a true b;
    and    = λa. λb. a b false;
    pair   = λf. λs. λb. b f s;
    first  = λp. p true;
    second = λp. p false;

    succ = λn.λs.λz. s (n s z);
    add  = λa.λb.λs.λz. a s (b s z);
    mul  = λa.λb.λs. a (b s);
    pow  = λa.λb. b a;

    iszero = λn. n (λx.false) true;

    zz   = pair 0 0;
    ss   = λp. pair (second p) (succ (second p));
    pred = λn. first (n ss zz);

    fact = fix (λf.λn. (iszero n) (λd.1) (λd.mul n (f (pred n))) λd.d);

    fix = λf.(λw.f (λv.w w v)) (λw.f (λv.w w v));

    """.stripMargin
//  curry   = λf.λx.λy. f x y;
//  uncurry = λf.λp. f (CAR p) (CDR p);


  def load(): Map[String, Expr] = {
    val parse = new LambdaParser()
    import parse.{ Success, NoSuccess }
    parse.definitions(source) match {
      case Success(lib:Map[String, Expr], _) => lib
      case NoSuccess(err, _) => println(err); Map[String, Expr]()
    }
  }
}

