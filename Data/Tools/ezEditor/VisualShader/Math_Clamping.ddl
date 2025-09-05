Node %Max
{
  string %Category { "Math/Clamping" }
  string %Color { "Yellow" }
  string %Docs { "Outputs the larger value for each component of the input." }

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
    string %Inline { "max(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "The larger of the two input values (component-wise)." }
  }
}

Node %Min
{
  string %Category { "Math/Clamping" }
  string %Color { "Yellow" }
  string %Docs { "Outputs the smaller value for each component of the input." }

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
    string %Inline { "min(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "The smaller of the two input values (component-wise)." }
  }
}

Node %Saturate
{
  string %Category { "Math/Clamping" }
  string %Color { "Yellow" }
  string %Docs { "Clamps each component of the input to the range zero to one." }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "saturate($in0)" }
    string %Tooltip { "Clamps the input to the range [0, 1] (component-wise)." }
  }
}

Node %Clamp
{
  string %Category { "Math/Clamping" }
  string %Color { "Yellow" }
  string %Docs { "Clamps each component of the input to the given minimum and maximum range." }

  InputPin %x
  {
    string %Type { "float" }
    string %Tooltip { "The value to clamp." }
  }

  InputPin %min
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The minimum value to clamp against." }
  }

  InputPin %max
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
    string %Tooltip { "The maximum value to clamp against." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "clamp($in0, ToSameType($in1, $in0), ToSameType($in2, $in0))" }
    string %Tooltip { "All output values will be clamped to be between Min and Max." }
  }
}
