import ifj26.*

def choose(condition: Boolean)(left: Int)(right: Int): Int =
  if condition then left else right

@main def main(): Unit =
  val values: List[Int] = List(3, 5)
  val selected = choose(values(0) == 3)(values(0))(values(1))
  for value <- values do
    print(toStr(value + selected))
