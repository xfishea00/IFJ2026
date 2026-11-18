import ifj26.*

@main def elseifexample(): Unit =
  val n = readInt()  // nemodifikovatelna promenna
  // Int je ve Scale 32b - vice nez 12! by preteklo.
  // V IFJ chovani v krajnich situacich (pre-/podteceni) prilis netestujeme,
  // ale je vhodne se k tomu nejak konkretne pri implementaci postavit.
  if n < 0 then
    print("Use an integer from 0 to 12.\n")
  else if n == 0 then
    print("Use an integer from 0 to 12.\n")
  else
    val iterative = factorialIterative(n)
    val recursive = factorialRecursive(n)
    print("iterative: ")
    print(iterative)
    print("\nrecursive: ")
    print(recursive)
    print("\n")
