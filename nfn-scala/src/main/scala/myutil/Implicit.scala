package myutil

import scala.util.{Failure, Success, Try}
import scala.concurrent.Future

object Implicit {
  /**
   * Converts a [[Try]] into a [[Future]]
   * @param t
   * @tparam T
   * @return
   */
  implicit def tryToFuture[T](t:Try[T]):Future[T] = {
    t match{
      case Success(s) => Future.successful(s)
      case Failure(ex) => Future.failed(ex)
    }
  }

  implicit def tryFutureToFuture[T](t:Try[Future[T]]):Future[T] = {
    t match{
      case Success(s) => s
      case Failure(ex) => Future.failed(ex)
    }
  }
}

