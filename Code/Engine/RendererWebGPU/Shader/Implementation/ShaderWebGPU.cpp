#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <Foundation/IO/OSFile.h>
#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Shader/ShaderWebGPU.h>

#include <webgpu/webgpu_cpp.h>

ezGALShaderWebGPU::ezGALShaderWebGPU(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description)
{
}

ezGALShaderWebGPU::~ezGALShaderWebGPU() = default;

void ezGALShaderWebGPU::SetDebugName(ezStringView sName) const
{
#if EZ_ENABLED(EZ_PLATFORM_WEB)
  const ezStringBuilder sLabel(sName);
#else
  const wgpu::NullableStringView sLabel(sName.GetStartPointer(), sName.GetElementCount());
#endif

  if (m_ShaderModuleVS)
  {
    m_ShaderModuleVS.SetLabel(sLabel);
  }
  if (m_ShaderModuleFS)
  {
    m_ShaderModuleFS.SetLabel(sLabel);
  }
  if (m_ShaderModuleCS)
  {
    m_ShaderModuleCS.SetLabel(sLabel);
  }
}

ezResult ezGALShaderWebGPU::InitPlatform(ezGALDevice* pDevice0)
{
  EZ_WEBGPU_TRACE();

  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "WebGPU can't create resources on a different thread.");

  ezGALDeviceWebGPU* pDevice = (ezGALDeviceWebGPU*)pDevice0;

  EZ_SUCCEED_OR_RETURN(CreateBindingMapping(true));

  m_bLoadedSuccessfully = true;

  EZ_SUCCEED_OR_RETURN(CreateShaderModule(pDevice, m_Description.m_ByteCodes[ezGALShaderStage::VertexShader].Borrow(), m_ShaderModuleVS));
  EZ_SUCCEED_OR_RETURN(CreateShaderModule(pDevice, m_Description.m_ByteCodes[ezGALShaderStage::PixelShader].Borrow(), m_ShaderModuleFS));
  EZ_SUCCEED_OR_RETURN(CreateShaderModule(pDevice, m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader].Borrow(), m_ShaderModuleCS));

  if (!m_bLoadedSuccessfully)
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALShaderWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  m_ShaderModuleVS = nullptr;
  m_ShaderModuleFS = nullptr;
  m_ShaderModuleCS = nullptr;

  return EZ_SUCCESS;
}

void ezGALShaderWebGPU::WGPUCompilationInfoCallback(WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* pCompilationInfo, void* userdata)
{
  if (status == WGPUCompilationInfoRequestStatus_Success && pCompilationInfo->messageCount > 0)
  {
    ((ezGALShaderWebGPU*)userdata)->ShaderCompilationInfo(status, pCompilationInfo);
  }
}

void ezGALShaderWebGPU::ShaderCompilationInfo(WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* pCompilationInfo)
{
  EZ_LOG_BLOCK("Shader Compilation Error");

  m_bLoadedSuccessfully = false;

  for (ezUInt32 i = 0; i < pCompilationInfo->messageCount; ++i)
  {
    const auto& msg = pCompilationInfo->messages[i];

    const char* szErrorStart = m_szCurSource + msg.offset;
    const char* szErrorEnd = szErrorStart + msg.length;
    const char* szLineEnd = ezStringUtils::FindSubString(szErrorStart, "\n");
    const char* szLineStart = ezStringUtils::FindLastSubString(m_szCurSource, "\n", szErrorStart);
    ezStringView sError(szErrorStart, szErrorEnd);
    ezStringView sLine(szLineStart, szLineEnd);
    sError.Trim();
    sLine.Trim();

    switch (msg.type)
    {
      case WGPUCompilationMessageType_Error:
        ezLog::Error("Line {}:{}: '{}' - '{}'", msg.lineNum, msg.linePos, msg.message, sError);
        if (!sLine.IsEmpty())
          ezLog::Error(sLine);
        break;
      case WGPUCompilationMessageType_Warning:
        ezLog::Warning("Line {}:{}: '{}' - '{}'", msg.lineNum, msg.linePos, msg.message, sError);
        if (!sLine.IsEmpty())
          ezLog::Warning(sLine);
        break;
      case WGPUCompilationMessageType_Info:
        ezLog::Info("Line {}:{}: '{}' - '{}'", msg.lineNum, msg.linePos, msg.message, sError);
        if (!sLine.IsEmpty())
          ezLog::Info(sLine);
        break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
}

ezResult ezGALShaderWebGPU::CreateShaderModule(ezGALDeviceWebGPU* pDevice, const ezGALShaderByteCode* pByteCode, wgpu::ShaderModule& shaderModule)
{
  if (pByteCode == nullptr)
    return EZ_SUCCESS;

  const ezStringBuilder sSource = ezStringView(static_cast<const char*>(pByteCode->GetByteCode()), pByteCode->GetSize());

  if (sSource.IsEmpty())
    return EZ_SUCCESS;

  {
    wgpu::ShaderModuleWGSLDescriptor shaderSourceDesc;
    shaderSourceDesc.code = sSource.GetData();

    wgpu::ShaderModuleDescriptor shaderModuleDesc;
    shaderModuleDesc.nextInChain = &shaderSourceDesc;

    m_szCurSource = sSource;
    shaderModule = pDevice->GetDevice().CreateShaderModule(&shaderModuleDesc);
    shaderModule.GetCompilationInfo(WGPUCompilationInfoCallback, this);
    m_szCurSource = nullptr;
  }

  return m_bLoadedSuccessfully ? EZ_SUCCESS : EZ_FAILURE;
}
