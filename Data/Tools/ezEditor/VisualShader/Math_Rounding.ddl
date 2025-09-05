Node %Floor
{
  string %Category { "Math/Rounding" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the closest whole integer number that's smaller or equal than the input value." }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "floor($in0)" }
    string %Tooltip { "The largest integer which is less than or equal to the input (component-wise)." }
  }
}

Node %Ceil
{
  string %Category { "Math/Rounding" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the closest whole integer number that's larger or equal than the input value." }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "ceil($in0)" }
    string %Tooltip { "The smallest integer which is greater than or equal to the input (component-wise)." }
  }
}

Node %Round
{
  string %Category { "Math/Rounding" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the closest whole integer." }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "round($in0)" }
    string %Tooltip { "Rounds the input to the closest integer value (component-wise)." }
  }
}

Node %Truncate
{
  string %Category { "Math/Rounding" }
  string %Color { "Yellow" }
  string %Docs { "For each component outputs the whole integer part by discarding the fractional part." }

  InputPin %a
  {
    string %Type { "float" }
  }

  OutputPin %result
  {
    string %Type { "float" }
    string %Inline { "trunc($in0)" }
    string %Tooltip { "Removes the fractional part of the input without rounding (component-wise)." }
  }
}
