import ifj26.*

@main def main(): Unit =
  val value = 1
  val result = value match
    case number: Int "integer"
    case _ => "other"
