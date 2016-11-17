Node %ConstantColor
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 0, 53, 91 }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 0, 53, 91 }
    string %Inline { "$prop0" }
  }

  Property %Color
  {
    string %Type { "color" }
    Color %Value { float { 1, 1, 1, 1 } }
  }
}

Node %Constant1
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 0, 53, 91 }

  OutputPin %Value
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 53, 91 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float" }
    float %Value { 0 }
  }
}

Node %Constant2
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 0, 53, 91 }

  OutputPin %Value
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 0, 53, 91 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float2" }
    float %Value { 0, 0 }
  }
}

Node %Constant3
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 0, 53, 91 }

  OutputPin %Value
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 0, 53, 91 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float3" }
    float %Value { 0, 0, 0 }
  }
}

Node %Constant4
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 0, 53, 91 }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 0, 53, 91 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float4" }
    float %Value { 0, 0, 0, 0 }
  }
}


