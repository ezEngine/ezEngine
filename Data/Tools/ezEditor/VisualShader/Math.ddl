
Node %Lerp
{
  string %Category { "Math/Interpolation" }
  string %Color { "Yellow" }

  InputPin %x
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  InputPin %y
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  InputPin %factor
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
    string %Tooltip { "How much to interpolate between x and y. At 0 the output is x, at 1 the output is y." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "lerp(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0), ToFloat1($in2))" }
    string %Tooltip { "Linear interpolation between x and y according to factor." }
  }
}

Node %Step
{
  string %Category { "Math/Interpolation" }
  string %Color { "Yellow" }

  InputPin %edge
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
    string %Tooltip { "The value to compare with 'x'." }
  }

  InputPin %x
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The value to compare with 'edge'." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "step(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "Returns 0 when x is smaller than edge, 1 otherwise." }
  }
}

Node %SmoothStep
{
  string %Category { "Math/Interpolation" }
  string %Color { "Yellow" }

  InputPin %edge0
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.3" }
  }

  InputPin %edge1
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.6" }
  }

  InputPin %x
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "smoothstep(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0), ToBiggerType(ToBiggerType($in2, $in0)), $in1))" }
    string %Tooltip { "Returns 0 when x is smaller than edge0, 1 when x is larger than edge1 and the hermite interpoliation in between." }
  }
}