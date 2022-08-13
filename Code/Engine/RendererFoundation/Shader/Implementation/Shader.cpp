#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

ezGALShader::ezGALShader(const ezGALShaderCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALShader::~ezGALShader() {}

ezDelegate<void(ezShaderUtils::ezBuiltinShaderType type, ezShaderUtils::ezBuiltinShader& out_shader)> ezShaderUtils::g_RequestBuiltinShaderCallback;

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_Shader);
