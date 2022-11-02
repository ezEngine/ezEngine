
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
