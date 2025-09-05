Node %Split
{
  string %Category { "Components" }
  string %Color { "Lime" }
  string %Docs { "Outputs each component of the input vector separately." }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "Value that should be split into components.\nCan be of any type, missing components output zero." }
  }

  OutputPin %x
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "ToFloat1($in0)" }
    string %Tooltip { "Returns the x component of the incoming data." }
  }

  OutputPin %y
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "ToFloat2($in0).y" }
    string %Tooltip { "Returns the y component of the incoming data. Zero if the incoming data has less than 2 components." }
  }

  OutputPin %z
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "ToFloat3($in0).z" }
    string %Tooltip { "Returns the z component of the incoming data. Zero if the incoming data has less than 3 components." }
  }

  OutputPin %w
  {
    string %Type { "float" }
    string %Inline { "ToFloat4Direction($in0).w" }
    string %Tooltip { "Returns the w component of the incoming data. Zero if the incoming data has less than 4 components." }
  }
}

Node %MergeFloat2
{
  string %Category { "Components" }
  string %Color { "Lime" }
  string %Docs { "Combines 2 single values into a 2-component vector. " }

  InputPin %x
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The first component of the incoming data is put into the x component of the output." }
  }

  InputPin %y
  {
    string %Type { "float" }
    string %Color { "Green" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The first component of the incoming data is put into the y component of the output." }
  }

  OutputPin %result
  {
      string %Type { "float" }
      unsigned_int8 %Color { 200, 200, 200 }
      string %Inline { "float2(ToFloat1($in0), ToFloat1($in1))" }
      string %Tooltip { "The first component of each input is put into the respective component of the output." }
    }
}

Node %MergeFloat3
{
  string %Category { "Components" }
  string %Color { "Lime" }
  string %Docs { "Combines 3 single values into a 3-component vector. " }

  InputPin %x
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The first component of the incoming data is put into the x component of the output." }
  }

  InputPin %y
  {
    string %Type { "float" }
    string %Color { "Green" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The first component of the incoming data is put into the y component of the output." }
  }

  InputPin %z
  {
    string %Type { "float" }
    string %Color { "Blue" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The first component of the incoming data is put into the z component of the output." }
  }

  OutputPin %result
  {
      string %Type { "float" }
      unsigned_int8 %Color { 200, 200, 200 }
      string %Inline { "float3(ToFloat1($in0), ToFloat1($in1), ToFloat1($in2))" }
      string %Tooltip { "The first component of each input is put into the respective component of the output." }
  }
}

Node %MergeFloat4
{
  string %Category { "Components" }
  string %Color { "Lime" }
  string %Docs { "Combines 4 single values into a 4-component vector. " }

  InputPin %x
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "The first component of the incoming data is put into the x component of the output." }
  }

  InputPin %y
  {
      string %Type { "float" }
      string %Color { "Green" }
      bool %Expose { true }
      string %DefaultValue { "0" }
      string %Tooltip { "The first component of the incoming data is put into the y component of the output." }
  }

  InputPin %z
  {
      string %Type { "float" }
      string %Color { "Blue" }
      bool %Expose { true }
      string %DefaultValue { "0" }
      string %Tooltip { "The first component of the incoming data is put into the z component of the output." }
  }

  InputPin %w
  {
      string %Type { "float" }
      bool %Expose { true }
      string %DefaultValue { "0" }
      string %Tooltip { "The first component of the incoming data is put into the w component of the output." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "float4(ToFloat1($in0), ToFloat1($in1), ToFloat1($in2), ToFloat1($in3))" }
    string %Tooltip { "The first component of each input is put into the respective component of the output." }
  }
}
