Node %Split
{
  string %Category { "Components" }
  unsigned_int8 %Color { 123, 137, 0 }

  InputPin %a
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "Value that should be split into components.\nCan be of any type, missing components output zero." }
  }

  OutputPin %x
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    string %Inline { "ToFloat1($in0)" }
    string %Tooltip { "Returns the x component of the incoming data." }
  }

  OutputPin %y
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 128, 0 }
    string %Inline { "ToFloat2($in0).y" }
    string %Tooltip { "Returns the y component of the incoming data. Zero if the incoming data has less than 2 components." }
  }

  OutputPin %z
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 0, 128 }
    string %Inline { "ToFloat3($in0).z" }
    string %Tooltip { "Returns the z component of the incoming data. Zero if the incoming data has less than 3 components." }
  }

  OutputPin %w
  {
    string %Type { "float" }
    unsigned_int8 %Color { 175, 175, 117 }
    string %Inline { "ToFloat4($in0).w" }
    string %Tooltip { "Returns the w component of the incoming data. Zero if the incoming data has less than 4 components." }
  }
}

Node %MergeFloat2
{
  string %Category { "Components" }
  unsigned_int8 %Color { 123, 137, 0 }

  InputPin %x
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    bool %Expose { true }
    string %Fallback { "0" }
    string %Tooltip { "The first component of the incoming data is put into the x component of the output." }
  }

  InputPin %y
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 128, 0 }
    bool %Expose { true }
    string %Fallback { "0" }
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
  unsigned_int8 %Color { 123, 137, 0 }

  InputPin %x
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    bool %Expose { true }
    string %Fallback { "0" }
    string %Tooltip { "The first component of the incoming data is put into the x component of the output." }
  }

  InputPin %y
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 128, 0 }
    bool %Expose { true }
    string %Fallback { "0" }
    string %Tooltip { "The first component of the incoming data is put into the y component of the output." }
  }

  InputPin %z
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 0, 128 }
    bool %Expose { true }
    string %Fallback { "0" }
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
  unsigned_int8 %Color { 123, 137, 0 }

  InputPin %x
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    bool %Expose { true }
    string %Fallback { "0" }
    string %Tooltip { "The first component of the incoming data is put into the x component of the output." }
  }

  InputPin %y
  {
      string %Type { "float" }
      unsigned_int8 %Color { 0, 128, 0 }
      bool %Expose { true }
      string %Fallback { "0" }
      string %Tooltip { "The first component of the incoming data is put into the y component of the output." }
  }

  InputPin %z
  {
      string %Type { "float" }
      unsigned_int8 %Color { 0, 0, 128 }
      bool %Expose { true }
      string %Fallback { "0" }
      string %Tooltip { "The first component of the incoming data is put into the z component of the output." }
  }

  InputPin %w
  {
      string %Type { "float" }
      unsigned_int8 %Color { 175, 175, 117 }
      bool %Expose { true }
      string %Fallback { "0" }
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
