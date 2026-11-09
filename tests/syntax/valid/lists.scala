import ifj26.*

@main def main(): Unit =
  val values: List[Int] = List(1, 2, 3)
  val empty: List[Int] = List()
  val extended = values ++ List(4)
  val first = extended(0)
  for value <- values do
    print(value)
  print(first)
  print(empty)
