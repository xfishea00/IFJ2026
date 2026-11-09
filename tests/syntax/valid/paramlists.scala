import ifj26.*

def answer: Int = 42
def answerWithParens(): Int = 42
def add(left: Int)(right: Int): Int = left + right
def offset()(value: Int): Int = value + 1

@main def main(): Unit =
  val constant = answer
  val explicitEmpty = answerWithParens()
  val sum = add(constant)(explicitEmpty)
  val adjusted = offset()(sum)
  print(adjusted)
