Node %Parameter1f
{
  string %NodeType { "Parameter" }
  string %Category { "Parameters" }
  string %Color { "Red" }
  string %Docs { "Outputs a 1-component value that can be configured on the material." }
  string %Title { "Param 1: {$ParamName}" }

  string %CodeMaterialParams { "float $prop0;" }
  string %CodeMaterialCB { "FLOAT1($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomValue1f" }
  }

  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "GetMaterialData($prop0)" }
  }
}

Node %Parameter2f
{
  string %NodeType { "Parameter" }
  string %Category { "Parameters" }
  string %Color { "Red" }
  string %Docs { "Outputs a 2-component value that can be configured on the material." }
  string %Title { "Param 2: {$ParamName}" }

  string %CodeMaterialParams { "float2 $prop0;" }
  string %CodeMaterialCB { "FLOAT2($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomValue2f" }
  }

  OutputPin %Value
  {
    string %Type { "float2" }
    string %Inline { "GetMaterialData($prop0)" }
  }
}

Node %Parameter3f
{
  string %NodeType { "Parameter" }
  string %Category { "Parameters" }
  string %Color { "Red" }
  string %Docs { "Outputs a 3-component value that can be configured on the material." }
  string %Title { "Param 3: {$ParamName}" }

  string %CodeMaterialParams { "float3 $prop0;" }
  string %CodeMaterialCB { "FLOAT3($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomValue3f" }
  }

  OutputPin %Value
  {
    string %Type { "float3" }
    string %Inline { "GetMaterialData($prop0)" }
  }
}

Node %Parameter4f
{
  string %NodeType { "Parameter" }
  string %Category { "Parameters" }
  string %Color { "Red" }
  string %Docs { "Outputs a 4-component value that can be configured on the material." }
  string %Title { "Param 4: {$ParamName}" }

  string %CodeMaterialParams { "float4 $prop0;" }
  string %CodeMaterialCB { "FLOAT4($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomValue4f" }
  }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "GetMaterialData($prop0)" }
  }
}

Node %ParameterColor
{
  string %NodeType { "Parameter" }
  string %Category { "Parameters" }
  string %Color { "Red" }
  string %Docs { "Outputs a color value that can be configured on the material." }
  string %Title { "Param Color: {$ParamName}" }

  string %CodeMaterialParams { "Color $prop0;" }
  string %CodeMaterialCB { "COLOR4F($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomColor" }
  }

  OutputPin %Value
  {
    string %Type { "color" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "GetMaterialData($prop0)" }
  }
}
