package myutil

import scala.util.{Failure, Success, Try}
import scala.concurrent.Future

object Implicit {
  // converts a try to a future
  implicit def tryToFuture[T](t:Try[T]):Future[T] = {
    t match{
      case Success(s) => Future.successful(s)
      case Failure(ex) => Future.failed(ex)
    }
  }
}
