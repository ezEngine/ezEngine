Node %ConstantColor
{
  string %Category { "Constants" }
  string %Color { "Orange" }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }

  Property %Color
  {
    string %Type { "color" }
    string %DefaultValue { "255, 255, 255, 255" }
  }
}

Node %Constant1
{
  string %Category { "Constants" }
  string %Color { "Orange" }

  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }
}

Node %Constant2
{
  string %Category { "Constants" }
  string %Color { "Orange" }

  OutputPin %Value
  {
    string %Type { "float2" }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float2" }
    string %DefaultValue { "0, 0" }
  }
}

Node %Constant3
{
  string %Category { "Constants" }
  string %Color { "Orange" }

  OutputPin %Value
  {
    string %Type { "float3" }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float3" }
    string %DefaultValue { "0, 0, 0" }
  }
}

Node %Constant4
{
  string %Category { "Constants" }
  string %Color { "Orange" }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float4" }
    string %DefaultValue { "0, 0, 0, 0" }
  }
}


