Node %Parameter1f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 128, 0, 0 }

  string %CodeMaterialParams { "float $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT1($prop0);" }

  Property %ParamName
  {
    string %Type { "string" }
    string %Value { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float" }
    float %Value { 0 }
  }

  OutputPin %Value
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}

Node %Parameter2f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 128, 0, 0 }

  string %CodeMaterialParams { "float2 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT2($prop0);" }

  Property %ParamName
  {
    string %Type { "string" }
    string %Value { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float2" }
    float %Value { 0, 0 }
  }

  OutputPin %Value
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}

Node %Parameter3f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 128, 0, 0 }

  string %CodeMaterialParams { "float3 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT3($prop0);" }

  Property %ParamName
  {
    string %Type { "string" }
    string %Value { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float3" }
    float %Value { 0, 0, 0 }
  }

  OutputPin %Value
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}

Node %Parameter4f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 128, 0, 0 }

  string %CodeMaterialParams { "float4 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT4($prop0);" }

  Property %ParamName
  {
    string %Type { "string" }
    string %Value { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float4" }
    float %Value { 0, 0, 0, 0 }
  }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}

Node %ParameterColor
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 128, 0, 0 }

  // TODO: Insert default value for color
  // (currently this creates @Default(float4(1,1,1))) which results in a parsing error
  string %CodeMaterialParams { "Color $prop0;" }
  string %CodeMaterialCB { "COLOR4F($prop0);" }

  Property %ParamName
  {
    string %Type { "string" }
    string %Value { "Parameter" }
  }

  Property %Default
  {
    string %Type { "color" }
    float %Value { 0,0,0 }
  }

  OutputPin %Value
  {
    string %Type { "color" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}
