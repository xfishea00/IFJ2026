import ifj26.*

@main def expressionEdges(): Unit =
  val arithmetic = -((2 + 3) * 4) + 1
  val comparison = 1 + 2 * 3 >= 7
  val logic = !false && (true || false)
  val concatenated = "prefix" ++ ":" ++ "suffix"
  print(arithmetic)
  print(comparison)
  print(logic)
  print(concatenated)
