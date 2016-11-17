Node %Parameter1
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 128, 0, 0 }

  string %CodeMaterialParams { "float $prop0 @Default($prop1);" }

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

  OutputPins %Value
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}