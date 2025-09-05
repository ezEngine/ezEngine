Node %Distance
{
  string %Category { "Math/Vector" }
  string %Color { "Yellow" }
  string %Docs { "Calculates the distance between the two vectors." }

  InputPin %a
  {
    string %Type { "float" }
  }

  InputPin %b
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "distance(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "The distance between the two input vectors." }
  }
}

Node %Length
{
  string %Category { "Math/Vector" }
  string %Color { "Yellow" }
  string %Docs { "Calculates the length of the vector." }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "length($in0)" }
    string %Tooltip { "The length of the input vector." }
  }
}

Node %Dot
{
  string %Category { "Math/Vector" }
  string %Color { "Yellow" }
  string %Docs { "Outputs the dot-product of the two vectors." }

  InputPin %a
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "dot(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0))" }
    string %Tooltip { "Dot product between a and b. A scalar value." }
  }
}

Node %Cross
{
  string %Category { "Math/Vector" }
  string %Color { "Yellow" }
  string %Docs { "Calculates the cross-product of the two vectors." }

  InputPin %a
  {
    string %Type { "float3" }
    string %DefaultValue { "0" }
  }

  InputPin %b
  {
    string %Type { "float3" }
    string %DefaultValue { "0" }
  }

  OutputPin %result
  {
    string %Type { "float3" }
    string %Inline { "cross(ToFloat3($in0), ToFloat3($in1))" }
    string %Tooltip { "Cross product between a and b. Inputs are converted to float3, result is float3." }
  }
}

Node %Normalize
{
  string %Category { "Math/Vector" }
  string %Color { "Yellow" }
  string %Docs { "Outputs the normalized input vector." }

  InputPin %vector
  {
    string %Type { "float" }
    string %Tooltip { "A vector. Avoid zero vectors to prevent NaN's." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "normalize($in0)" }
    string %Tooltip { "The normalized vector." }
  }
}
