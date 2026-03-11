Node %StencilState
{
  string %NodeType { "ShaderState" }
  string %Category { "RenderStates" }
  string %Color { "Blue" }
  string %Docs { "Configures the stencil buffer state for stencil testing operations" }

  Property %StencilEnable
  {
    string %Type { "bool" }
    string %DefaultValue { "true" }
  }

  Property %StencilCompareFunc
  {
    string %Type { "enum" }
    string %DefaultValue { "CompareFunc_Equal" }
    string %EnumValues { "CompareFunc_Never, CompareFunc_Less, CompareFunc_Equal, CompareFunc_LessEqual, CompareFunc_Greater, CompareFunc_NotEqual, CompareFunc_GreaterEqual, CompareFunc_Always" }
  }

  Property %StencilPassOp
  {
    string %Type { "enum" }
    string %DefaultValue { "StencilOp_Keep" }
    string %EnumValues { "StencilOp_Keep, StencilOp_Zero, StencilOp_Replace, StencilOp_IncrementSaturated, StencilOp_DecrementSaturated, StencilOp_Invert, StencilOp_Increment, StencilOp_Decrement" }
  }

  Property %StencilFailOp
  {
    string %Type { "enum" }
    string %DefaultValue { "StencilOp_Keep" }
    string %EnumValues { "StencilOp_Keep, StencilOp_Zero, StencilOp_Replace, StencilOp_IncrementSaturated, StencilOp_DecrementSaturated, StencilOp_Invert, StencilOp_Increment, StencilOp_Decrement" }
  }

  Property %StencilDepthFailOp
  {
    string %Type { "enum" }
    string %DefaultValue { "StencilOp_Keep" }
    string %EnumValues { "StencilOp_Keep, StencilOp_Zero, StencilOp_Replace, StencilOp_IncrementSaturated, StencilOp_DecrementSaturated, StencilOp_Invert, StencilOp_Increment, StencilOp_Decrement" }
  }

  Property %StencilReadMask
  {
    string %Type { "int" }
    string %DefaultValue { "255" }
  }

  Property %StencilWriteMask
  {
    string %Type { "int" }
    string %DefaultValue { "255" }
  }

  Property %StencilRef
  {
    string %Type { "int" }
    string %DefaultValue { "0" }
  }

  string %CodeRenderStates { "StencilEnable = $prop0
StencilCompareFunc = $prop1
StencilPassOp = $prop2
StencilFailOp = $prop3
StencilDepthFailOp = $prop4
StencilReadMask = $prop5
StencilWriteMask = $prop6
StencilRef = $prop7" }
}

Node %DepthState
{
  string %NodeType { "ShaderState" }
  string %Category { "RenderStates" }
  string %Color { "Blue" }
  string %Docs { "Configures depth buffer testing and writing" }

  Property %DepthEnable
  {
    string %Type { "bool" }
    string %DefaultValue { "true" }
  }

  Property %DepthWrite
  {
    string %Type { "bool" }
    string %DefaultValue { "true" }
  }

  Property %DepthTestFunc
  {
    string %Type { "enum" }
    string %DefaultValue { "CompareFunc_LessEqual" }
    string %EnumValues { "CompareFunc_Never, CompareFunc_Less, CompareFunc_Equal, CompareFunc_LessEqual, CompareFunc_Greater, CompareFunc_NotEqual, CompareFunc_GreaterEqual, CompareFunc_Always" }
  }

  string %CodeRenderStates { "DepthEnable = $prop0
DepthWrite = $prop1
DepthTestFunc = $prop2" }
}

Node %BlendState
{
  string %NodeType { "ShaderState" }
  string %Category { "RenderStates" }
  string %Color { "Blue" }
  string %Docs { "Configures color blending for render target 0" }

  Property %BlendingEnabled
  {
    string %Type { "bool" }
    string %DefaultValue { "false" }
  }

  Property %SourceBlend
  {
    string %Type { "enum" }
    string %DefaultValue { "Blend_SrcAlpha" }
    string %EnumValues { "Blend_Zero, Blend_One, Blend_SrcColor, Blend_InvSrcColor, Blend_SrcAlpha, Blend_InvSrcAlpha, Blend_DestAlpha, Blend_InvDestAlpha, Blend_DestColor, Blend_InvDestColor, Blend_SrcAlphaSaturated, Blend_BlendFactor, Blend_InvBlendFactor" }
  }

  Property %DestBlend
  {
    string %Type { "enum" }
    string %DefaultValue { "Blend_InvSrcAlpha" }
    string %EnumValues { "Blend_Zero, Blend_One, Blend_SrcColor, Blend_InvSrcColor, Blend_SrcAlpha, Blend_InvSrcAlpha, Blend_DestAlpha, Blend_InvDestAlpha, Blend_DestColor, Blend_InvDestColor, Blend_SrcAlphaSaturated, Blend_BlendFactor, Blend_InvBlendFactor" }
  }

  Property %BlendOp
  {
    string %Type { "enum" }
    string %DefaultValue { "BlendOp_Add" }
    string %EnumValues { "BlendOp_Add, BlendOp_Subtract, BlendOp_RevSubtract, BlendOp_Min, BlendOp_Max" }
  }

  Property %SourceBlendAlpha
  {
    string %Type { "enum" }
    string %DefaultValue { "Blend_One" }
    string %EnumValues { "Blend_Zero, Blend_One, Blend_SrcColor, Blend_InvSrcColor, Blend_SrcAlpha, Blend_InvSrcAlpha, Blend_DestAlpha, Blend_InvDestAlpha, Blend_DestColor, Blend_InvDestColor, Blend_SrcAlphaSaturated, Blend_BlendFactor, Blend_InvBlendFactor" }
  }

  Property %DestBlendAlpha
  {
    string %Type { "enum" }
    string %DefaultValue { "Blend_InvSrcAlpha" }
    string %EnumValues { "Blend_Zero, Blend_One, Blend_SrcColor, Blend_InvSrcColor, Blend_SrcAlpha, Blend_InvSrcAlpha, Blend_DestAlpha, Blend_InvDestAlpha, Blend_DestColor, Blend_InvDestColor, Blend_SrcAlphaSaturated, Blend_BlendFactor, Blend_InvBlendFactor" }
  }

  Property %BlendOpAlpha
  {
    string %Type { "enum" }
    string %DefaultValue { "BlendOp_Add" }
    string %EnumValues { "BlendOp_Add, BlendOp_Subtract, BlendOp_RevSubtract, BlendOp_Min, BlendOp_Max" }
  }

  string %CodeRenderStates { "BlendingEnabled = $prop0
SourceBlend = $prop1
DestBlend = $prop2
BlendOp = $prop3
SourceBlendAlpha = $prop4
DestBlendAlpha = $prop5
BlendOpAlpha = $prop6" }
}

Node %RasterizerState
{
  string %NodeType { "ShaderState" }
  string %Category { "RenderStates" }
  string %Color { "Blue" }
  string %Docs { "Configures common rasterizer settings" }

  Property %CullMode
  {
    string %Type { "enum" }
    string %DefaultValue { "CullMode_Back" }
    string %EnumValues { "CullMode_None, CullMode_Front, CullMode_Back" }
  }

  Property %WireFrame
  {
    string %Type { "bool" }
    string %DefaultValue { "false" }
  }

  string %CodeRenderStates { "CullMode = $prop0
WireFrame = $prop1" }
}
