import ifj26.*

def absolute(value: Int): Int = if value < 0 then 0 - value else value

@main def main(): Unit =
  val value = absolute(-4)
  for i <- 1 to value by 2 do
    print(i)
  for
    i <- 0 until value
    if i != 1
    if i < 4
  do
    print(i)
