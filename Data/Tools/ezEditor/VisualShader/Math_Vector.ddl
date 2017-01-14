Node %Distance
{
  string %Category { "Math/Vector" }
  unsigned_int8 %Color { 216, 86, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
  }

  InputPin %b
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "distance(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "The distance between the two input vectors." }
  }
}

Node %Length
{
  string %Category { "Math/Vector" }
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
    string %Inline { "length($in0)" }
    string %Tooltip { "The length of the input vector." }
  }
}

Node %Dot
{
  string %Category { "Math/Vector" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 50, 50 }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float" }
    unsigned_int8 %Color { 50, 128, 50 }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "dot(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "Dot product between a and b. A scalar value." }
  }
}

Node %Cross
{
  string %Category { "Math/Vector" }
  unsigned_int8 %Color { 183, 153, 0 }

  InputPin %a
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 50, 50 }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 50, 128, 50 }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "cross(ToFloat3($in0), ToFloat3($in1))" }
    string %Tooltip { "Cross product between a and b. Inputs are converted to float3, result is float3." }
  }
}

Node %Normalize
{
  string %Category { "Math/Vector" }
  unsigned_int8 %Color { 216, 86, 0 }

  InputPin %vector
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "A vector. Avoid zero vectors to prevent NaN's." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "normalize($in0)" }
    string %Tooltip { "The normalized vector." }
  }
}
