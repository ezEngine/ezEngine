Node %Add
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "(ToBiggerType($in0, $in1) + ToBiggerType($in1, $in0))" }
    string %Tooltip { "a + b" }
  }
}

Node %Subtract
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "(ToBiggerType($in0, $in1) - ToBiggerType($in1, $in0))" }
    string %Tooltip { "a - b" }
  }
}

Node %Multiply
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  InputPin %b
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "(ToBiggerType($in0, $in1) * ToBiggerType($in1, $in0))" }
    string %Tooltip { "a * b (component-wise)" }
  }
}

Node %Divide
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 216, 86, 0 }

  InputPin %a
    {
      string %Type { "float" }
      unsigned_int8 %Color { 128, 50, 50 }
      bool %Expose { true }
      string %DefaultValue { "1" }
    }

  InputPin %b
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "(ToBiggerType($in0, $in1) / ToBiggerType($in1, $in0))" }
    string %Tooltip { "a / b (component-wise)" }
  }
}

Node %Fraction
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "frac($in0)" }
    string %Tooltip { "The fractional part of the input (component-wise)." }
  }
}

Node %Abs
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "abs($in0)" }
    string %Tooltip { "The absolute value of the input (component-wise)." }
  }
}

Node %Sign
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "sign($in0)" }
    string %Tooltip { "Outputs the sign of the input (component-wise)." }
  }
}

Node %Sqrt
{
  string %Category { "Math/Basic" }
  unsigned_int8 %Color { 216, 86, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "sqrt($in0)" }
    string %Tooltip { "The square root of the input (component-wise)." }
  }
}

Node %Negate
{
  string %Category { "Math/Vector" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "-$in0" }
    string %Tooltip { "Negated input value." }
  }
}
