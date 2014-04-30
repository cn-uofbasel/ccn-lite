package ccn

import scala.collection.mutable

import com.typesafe.scalalogging.slf4j.Logging

import ccn.packet._

/**
 * Created by basil on 04/04/14.
 */
case class ContentStore() extends Logging {
  private val cs: mutable.Map[CCNName, Content] = mutable.Map()

  def add(content: Content) = {
    cs += (content.name -> content)
  }

  def find(name: CCNName):Option[Content] = cs.get(name)

  def remote(name: CCNName): Unit = cs -= name
}
