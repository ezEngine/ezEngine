Node %Sine
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "The angle value in radians." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "sin($in0)" }
    string %Tooltip { "The sine of the input (component-wise)." }
  }
}

Node %Cosine
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "The angle value in radians." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "cos($in0)" }
    string %Tooltip { "The cosine of the input (component-wise)." }
  }
}

Node %Exp
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "exp($in0)" }
    string %Tooltip { "The base-e exponential (component-wise)." }
  }
}

Node %Exp2
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "exp2($in0)" }
    string %Tooltip { "The base-2 exponential (component-wise)." }
  }
}

Node %Log
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "log($in0)" }
    string %Tooltip { "The base-e logarithm of a. (component-wise)." }
  }
}

Node %Log2
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "log2($in0)" }
    string %Tooltip { "The base-2 logarithm of a. (component-wise)." }
  }
}

Node %Log10
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "log10($in0)" }
    string %Tooltip { "The base-10 logarithm of a. (component-wise)." }
  }
}

Node %Pow
{
  string %Category { "Math/Trigonometry" }
  unsigned_int8 %Color { 183, 36, 3 }

  InputPin %Base
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
    string %Tooltip { "" }
    bool %Expose { true }
    string %Fallback { "1" }
  }

  InputPin %Exponent
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    string %Tooltip { "" }
    bool %Expose { true }
    string %Fallback { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "pow(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "Base raised to the power of Exponent. (component-wise)." }
  }
}
