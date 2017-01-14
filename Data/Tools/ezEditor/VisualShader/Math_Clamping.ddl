Node %Max
{
  string %Category { "Math/Clamping" }
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
    string %Inline { "max(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "The larger of the two input values (component-wise)." }
  }
}

Node %Min
{
  string %Category { "Math/Clamping" }
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
    string %Inline { "min(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "The smaller of the two input values (component-wise)." }
  }
}

Node %Saturate
{
  string %Category { "Math/Clamping" }
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
    string %Inline { "saturate($in0)" }
    string %Tooltip { "Clamps the input to the range [0, 1] (component-wise)." }
  }
}

Node %Clamp
{
  string %Category { "Math/Clamping" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %x
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 128, 50 }
    string %Tooltip { "The value to clamp." }
  }

  InputPin %min
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The minimum value to clamp against." }
  }

  InputPin %max
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    bool %Expose { true }
    string %DefaultValue { "1" }
    string %Tooltip { "The maximum value to clamp against." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "clamp($in0, ToSameType($in1, $in0), ToSameType($in2, $in0))" }
    string %Tooltip { "All output values will be clamped to be between Min and Max." }
  }
}
