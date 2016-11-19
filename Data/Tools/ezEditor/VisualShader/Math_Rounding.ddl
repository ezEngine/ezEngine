Node %Floor
{
  string %Category { "Math/Rounding" }
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
    string %Inline { "floor($in0)" }
    string %Tooltip { "The largest integer which is less than or equal to the input (component-wise)." }
  }
}

Node %Ceil
{
  string %Category { "Math/Rounding" }
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
    string %Inline { "ceil($in0)" }
    string %Tooltip { "The smallest integer which is greater than or equal to the input (component-wise)." }
  }
}

Node %Round
{
  string %Category { "Math/Rounding" }
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
    string %Inline { "round($in0)" }
    string %Tooltip { "Rounds the input to the closest integer value (component-wise)." }
  }
}

Node %Truncate
{
  string %Category { "Math/Rounding" }
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
    string %Inline { "trunc($in0)" }
    string %Tooltip { "Removes the fractional part of the input without rounding (component-wise)." }
  }
}
