#include <PCH.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerHLSL, ezShaderProgramCompiler, 1, ezRTTIDefaultAllocator<ezShaderCompilerHLSL>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezShaderCompilerHLSLPlugin);

ezDynamicArray<ezUInt8> CompileDXShader(const char* source, const char* profile, const char* entryPoint)
{
  ID3DBlob* ResultBlob = nullptr;
  ID3DBlob* ErrorBlob = nullptr;

  if (FAILED(D3DCompile(source, strlen(source), "<file>", nullptr, nullptr, entryPoint, profile, 0, 0, &ResultBlob, &ErrorBlob)))
  {
    const char* szError = (const char*) ErrorBlob->GetBufferPointer();

    EZ_ASSERT(false, "Shader compilation failed for profile %s: %s", profile, szError);

    return nullptr;
  }

  ezDynamicArray<ezUInt8> r;

  if (ResultBlob != nullptr)
  {
    r.SetCount((ezUInt32) ResultBlob->GetBufferSize());
    ezMemoryUtils::Copy(&r[0], (ezUInt8*) ResultBlob->GetBufferPointer(), r.GetCount());
    ResultBlob->Release();
  }

  if (ErrorBlob != nullptr)
  {
    ErrorBlob->Release();
  }

  return r;
}

const char* GetProfileName(const char* szPlatform, ezGALShaderStage::Enum Stage)
{
  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM40_93"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::VertexShader:    return "vs_4_0_level_9_3";
    case ezGALShaderStage::PixelShader:     return "ps_4_0_level_9_3";
    default: break;
    }
  }

  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM40"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::VertexShader:    return "vs_4_0";
    case ezGALShaderStage::GeometryShader:  return "gs_4_0";
    case ezGALShaderStage::PixelShader:     return "ps_4_0";
    case ezGALShaderStage::ComputeShader:   return "cs_4_0";
    default: break;
    }
  }

  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM41"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::GeometryShader:  return "gs_4_0";
    case ezGALShaderStage::VertexShader:    return "vs_4_1";
    case ezGALShaderStage::PixelShader:     return "ps_4_1";
    case ezGALShaderStage::ComputeShader:   return "cs_4_1";
    default: break;
    }
  }

  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM50"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::VertexShader:    return "vs_5_0";
    case ezGALShaderStage::HullShader:      return "hs_5_0";
    case ezGALShaderStage::DomainShader:    return "ds_5_0";
    case ezGALShaderStage::GeometryShader:  return "gs_5_0";
    case ezGALShaderStage::PixelShader:     return "ps_5_0";
    case ezGALShaderStage::ComputeShader:   return "cs_5_0";
    default: break;
    }
  }

  EZ_REPORT_FAILURE("Unknown Platform '%s' or Stage %i", szPlatform, Stage);
  return "";
}

ezResult ezShaderCompilerHLSL::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // shader already compiled
    if (!inout_Data.m_StageBinary[stage].m_ByteCode.IsEmpty())
    {
      ezLog::Info("Shader for stage %u is already compiled.", stage);
      continue;
    }

    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(inout_Data.m_szShaderSource[stage]);

    if (uiLength > 0)
    {
      inout_Data.m_StageBinary[stage].m_ByteCode = CompileDXShader(inout_Data.m_szShaderSource[stage], GetProfileName(inout_Data.m_szPlatform, (ezGALShaderStage::Enum) stage), "main");
    }
  }

  return EZ_SUCCESS;
}

