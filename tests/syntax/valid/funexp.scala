import ifj26.*

def increment(value: Int): Int = value + 1
def combine(left: Int, right: Int): Int = left + right

@main def main(): Unit =
  val result = combine(increment(1), increment(increment(2)))
  print(toStr(result))
