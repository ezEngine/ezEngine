Node %Sine
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the sine of the given angle (given in Radians)." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "The angle value in Radians." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "sin($in0)" }
    string %Tooltip { "The sine of the input (component-wise)." }
  }
}

Node %Cosine
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the cosine of the given angle (given in Radians)." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "The angle value in Radians." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "cos($in0)" }
    string %Tooltip { "The cosine of the input (component-wise)." }
  }
}

Node %Exp
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the base-e exponential." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "exp($in0)" }
    string %Tooltip { "The base-e exponential (component-wise)." }
  }
}

Node %Exp2
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the base-2 exponential." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "exp2($in0)" }
    string %Tooltip { "The base-2 exponential (component-wise)." }
  }
}

Node %Log
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the base-e logarithm." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "log($in0)" }
    string %Tooltip { "The base-e logarithm of a. (component-wise)." }
  }
}

Node %Log2
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the base-2 logarithm." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "log2($in0)" }
    string %Tooltip { "The base-2 logarithm of a. (component-wise)." }
  }
}

Node %Log10
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the base-10 logarithm." }

  InputPin %a
  {
    string %Type { "float" }
    string %Tooltip { "" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "log10($in0)" }
    string %Tooltip { "The base-10 logarithm of a. (component-wise)." }
  }
}

Node %Pow
{
  string %Category { "Math/Trigonometry" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs 'Base' to the power of 'Exponent'." }

  InputPin %Base
  {
    string %Type { "float" }
    string %Tooltip { "" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  InputPin %Exponent
  {
    string %Type { "float" }
    string %Tooltip { "" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "pow(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "Base raised to the power of Exponent. (component-wise)." }
  }
}
