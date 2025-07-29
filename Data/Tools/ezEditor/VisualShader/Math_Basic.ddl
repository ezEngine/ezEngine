Node %Add
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "(ToBiggerType($in0, $in1) + ToBiggerType($in1, $in0))" }
    string %Tooltip { "a + b" }
  }
}

Node %Subtract
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "(ToBiggerType($in0, $in1) - ToBiggerType($in1, $in0))" }
    string %Tooltip { "a - b" }
  }
}

Node %Multiply
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  InputPin %b
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "(ToBiggerType($in0, $in1) * ToBiggerType($in1, $in0))" }
    string %Tooltip { "a * b (component-wise)" }
  }
}

Node %Divide
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
    {
      string %Type { "float" }
      bool %Expose { true }
      string %DefaultValue { "1" }
    }

  InputPin %b
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "(ToBiggerType($in0, $in1) / ToBiggerType($in1, $in0))" }
    string %Tooltip { "a / b (component-wise)" }
  }
}

Node %Modulo
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
    {
      string %Type { "float" }
      bool %Expose { true }
      string %DefaultValue { "1" }
    }

  InputPin %b
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "(ToBiggerType($in0, $in1) % ToBiggerType($in1, $in0))" }
    string %Tooltip { "a modulo b (component-wise)" }
  }
}

Node %Fraction
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "frac($in0)" }
    string %Tooltip { "The fractional part of the input (component-wise)." }
  }
}

Node %Abs
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "abs($in0)" }
    string %Tooltip { "The absolute value of the input (component-wise)." }
  }
}

Node %Sign
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "sign($in0)" }
    string %Tooltip { "Outputs the sign of the input (component-wise)." }
  }
}

Node %Sqrt
{
  string %Category { "Math/Basic" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "sqrt($in0)" }
    string %Tooltip { "The square root of the input (component-wise)." }
  }
}

Node %Negate
{
  string %Category { "Math/Vector" }
  string %Color { "Yellow" }

  InputPin %a
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "-$in0" }
    string %Tooltip { "Negated input value." }
  }
}
