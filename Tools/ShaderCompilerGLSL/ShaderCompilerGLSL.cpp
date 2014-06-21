#include <PCH.h>
#include <ShaderCompilerGLSL/ShaderCompilerGLSL.h>

void OnLoadPlugin(bool bReloading)    { }
void OnUnloadPlugin(bool bReloading)  { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezShaderCompilerGLSLPlugin);

ezShaderCompilerGLSL Compiler;

ezResult ezShaderCompilerGLSL::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(inout_Data.m_szShaderSource[stage]);

    if (uiLength > 0)
      inout_Data.m_CompiledShader.m_ByteCodes[stage] = new ezGALShaderByteCode(inout_Data.m_szShaderSource[stage], uiLength + 1);
  }

  return EZ_SUCCESS;
}

