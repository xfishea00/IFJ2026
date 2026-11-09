import ifj26.*

def negate(value: Boolean): Boolean = !value

@main def main(): Unit =
  val enabled: Boolean = negate(false) && (true || false)
  if enabled then
    print("enabled")
