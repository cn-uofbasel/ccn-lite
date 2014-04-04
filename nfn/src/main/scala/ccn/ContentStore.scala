package ccn

import scala.collection.mutable

import com.typesafe.scalalogging.slf4j.Logging

import ccn.packet.Content

/**
 * Created by basil on 04/04/14.
 */
case class ContentStore() extends Logging {
  private val cs: mutable.Map[Seq[String], Content] = mutable.Map()

  def add(content: Content) = {
    cs += (content.name -> content)
  }

  def find(name: Seq[String]):Option[Content] = cs.get(name)

  def remote(name: Seq[String]): Unit = cs -= name
}
