import ifj26.*

// ============================================================================
// IFJ26 Trailing Comma Test Suite
// Covers all 4 valid contexts specified in Section 1.2:
//   1. Data class attributes (ALGEBDT)
//   2. Function formal parameters (including multi-parameter lists / PARAMLISTS)
//   3. Function call arguments (including multi-argument lists / PARAMLISTS)
//   4. List constructor elements (LISTS)
//
// Also tests:
//   - Whitespace and comments between the trailing comma, newline, and ')'
//   - Multi-line closing parenthesis on the same indentation level as the header
//   - Single-element lists/params/args with a trailing comma
//   - Invalid cases (commented out at the bottom) that MUST trigger Syntax Error 2
// ============================================================================

// 1. Data class attributes (ALGEBDT) with trailing comma
case class Point3D(
    x: Int,
    y: Int,
    z: Int, // trailing comma followed by newline and ')'
)

// Single-attribute data class with trailing comma, whitespace, and block comment
case class Wrapper(
    value: String,   // line comment after trailing comma
    /* block comment before closing paren */
)

// 2. Function formal parameters with trailing comma (single-line items, newline before ')')
def sumTwo(a: Int, b: Int,
): Int =
    a + b

// Multi-line parameters with trailing comma and nested indentation
def formatCoord(
    label: String,
    pt: Point3D,
): String =
    label + ": (" + toStr(pt.x) + ", " + toStr(pt.y) + ", " + toStr(pt.z) + ")"

// Multiple parameter lists (PARAMLISTS) where each non-empty list uses a trailing comma
def curriedCalc(
    a: Int,
    b: Int,
)(
    multiplier: Int,
): Int =
    (a + b) * multiplier

@main def testTrailingCommas(): Unit =
    // 3. Function call arguments with trailing commas
    val s = sumTwo(
        10,
        20,
    )

    // Constructor call for data class (syntactically a function call) with trailing comma
    val origin = Point3D(
        0,
        0,
        0, // trailing comma with inline comment
    )

    // Single-argument call with trailing comma and multi-line block comment before ')'
    val wrapped = Wrapper(
        "hello",
        /* comment spanning
           multiple lines */
    )

    // Chained argument lists (PARAMLISTS) with trailing commas in both groups
    val total = curriedCalc(
        s,
        5,
    )(
        2,
    )

    // 4. List constructor elements (LISTS) with trailing commas
    val numbers: List[Int] = List(
        1,
        2,
        3,
    )

    // Single-element list with trailing comma
    val singleItem: List[String] = List(
        "only-one",
    )

    // Nested list constructors with trailing commas at both levels
    val matrix: List[List[Int]] = List(
        List(
            1,
            2,
        ),
        List(
            3,
            4,
        ),
    )

    print(formatCoord(wrapped.value, origin))
    print("\n")
    print(total)
    print("\n")
    print(numbers)
    print("\n")
    print(singleItem)
    print("\n")
    print(matrix)
    print("\n")

// ============================================================================
// INVALID TRAILING COMMA SCENARIOS (Each must fail with Syntax Error 2)
// Uncomment one at a time to test negative parser behavior:
// ============================================================================

// Invalid 1: Trailing comma on the SAME line as ')' (no newline between ',' and ')')
// def badParam(a: Int,): Int = a

// Invalid 2: Trailing comma in an EMPTY parameter list
// def emptyParamComma(,): Unit = ()

// Invalid 3: Trailing comma in function call on the SAME line as ')'
// val badCall = sumTwo(1, 2,)

// Invalid 4: Trailing comma in EMPTY argument list
// val badEmptyCall = testTrailingCommas(,)

// Invalid 5: Trailing comma in List() on the SAME line as ')'
// val badList = List(1, 2,)

// Invalid 6: Trailing comma in EMPTY List(,)
// val badEmptyList = List(,)

// Invalid 7: Trailing comma in case class on the SAME line as ')'
// case class BadClass(x: Int,)
