import ifj26.*

type Scalar = Int | Double | String
case class Point(x: Int, y: Int)

def describe(value: Scalar | Point): String =
  value match
    case number: Int => toStr(number) + "a"
    case number: Double => toStr(number)
    case text: String => text
    case Point(x, y) => toStr(x) + ":" + toStr(y)
    case _ => "other"

@main def algebdt(): Unit =
  val point = Point(2, 3)
  print(describe(point))
