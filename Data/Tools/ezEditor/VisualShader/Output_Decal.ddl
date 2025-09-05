Node %DecalOutput
{
  string %Category { "Output" }
  string %Color { "Grape" }
  string %NodeType { "Main" }
  string %Docs { "Output node for projected materials such as decals and lights." }

  string %CodePermutations { "" }

  string %CheckPermutations {""}

  string %CodeRenderStates { "#include <Shaders/Decals/DecalState.h>" }
  string %CodeVertexShader { "

#include <Shaders/Decals/DecalVertexShader.h>

VS_OUT main(VS_IN Input)
{
  return FillVertexData(Input);
}

" }

  string %CodeMaterialParams { "" }

  string %CodePixelDefines { "" }

  string %CodePixelIncludes { "
#include <Shaders/Decals/DecalPixelShader.h>
#include <Shaders/Common/VisualShaderUtil.h>
" }

  string %CodePixelSamplers { "" }
  string %CodePixelConstants { "" }
  string %CodeMaterialConstants { "

  // Insert custom Visual Shader parameters here
  VSE_CONSTANTS
" }

  string %CodePixelBody { "
  
float3 GetBaseColor()
{
  return ToColor3($in0);
}

float GetOpacity()
{
  return saturate(ToFloat1($in1));
}

" }

  // Pin 0
  InputPin %BaseColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }

  // Pin 1
  InputPin %Opacity
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
}
