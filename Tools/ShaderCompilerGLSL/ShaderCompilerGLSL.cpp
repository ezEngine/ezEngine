#include <PCH.h>
#include <ShaderCompilerGLSL/ShaderCompilerGLSL.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerGLSL, ezShaderProgramCompiler, 1, ezRTTIDefaultAllocator<ezShaderCompilerGLSL>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

void OnLoadPlugin(bool bReloading)    { }
void OnUnloadPlugin(bool bReloading)  { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezShaderCompilerGLSLPlugin);

ezResult ezShaderCompilerGLSL::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // shader already compiled
    if (!inout_Data.m_StageBinary[stage].m_ByteCode.IsEmpty())
      continue;

    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(inout_Data.m_szShaderSource[stage]);

    if (uiLength > 0)
    {
      inout_Data.m_StageBinary[stage].m_ByteCode.Reserve(uiLength + 1);
      inout_Data.m_StageBinary[stage].m_ByteCode.PushBackRange(ezArrayPtr<const ezUInt8>((const ezUInt8*) inout_Data.m_szShaderSource[stage], uiLength + 1));
    }
  }

  return EZ_SUCCESS;
}

