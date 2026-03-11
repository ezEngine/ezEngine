#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>



//////////////////////////////////////////////////////////////////////////

ezMap<ezUInt32, ezShaderStageBinary> ezShaderStageBinary::s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
ezMutex ezShaderStageBinary::s_ShaderStageBinariesLock;

ezShaderStageBinary::ezShaderStageBinary() = default;

ezShaderStageBinary::~ezShaderStageBinary()
{
  m_pGALByteCode = nullptr;
}

ezResult ezShaderStageBinary::Write(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = ezShaderStageBinary::VersionCurrent;

  // ezShaderStageBinary
  inout_stream << uiVersion;
  inout_stream << m_uiSourceHash;

  // ezGALShaderByteCode
  inout_stream << m_pGALByteCode->m_uiTessellationPatchControlPoints;
  inout_stream << m_pGALByteCode->m_Stage;
  inout_stream << m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  const ezUInt32 uiByteCodeSize = m_pGALByteCode->m_ByteCode.GetCount();
  inout_stream << uiByteCodeSize;
  if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize).Failed())
    return EZ_FAILURE;

  // m_ShaderResourceBindings
  const ezUInt16 uiResources = static_cast<ezUInt16>(m_pGALByteCode->m_ShaderResourceBindings.GetCount());
  inout_stream << uiResources;
  for (const auto& r : m_pGALByteCode->m_ShaderResourceBindings)
  {
    inout_stream << r.m_ResourceType;
    inout_stream << r.m_TextureType;
    inout_stream << r.m_Stages;
    inout_stream << r.m_iBindGroup;
    inout_stream << r.m_iSlot;
    inout_stream << r.m_uiArraySize;
    inout_stream << r.m_sName.GetData();
    const bool bHasLayout = r.m_pLayout != nullptr;
    inout_stream << bHasLayout;
    if (bHasLayout)
    {
      EZ_SUCCEED_OR_RETURN(Write(inout_stream, *r.m_pLayout));
    }
  }

  // m_ShaderVertexInput
  const ezUInt16 uiVertexInputs = static_cast<ezUInt16>(m_pGALByteCode->m_ShaderVertexInput.GetCount());
  inout_stream << uiVertexInputs;
  for (const auto& v : m_pGALByteCode->m_ShaderVertexInput)
  {
    inout_stream << v.m_eSemantic;
    inout_stream << v.m_eFormat;
    inout_stream << v.m_uiLocation;
  }

  return EZ_SUCCESS;
}


ezResult ezShaderStageBinary::Write(ezStreamWriter& inout_stream, const ezShaderConstantBufferLayout& layout) const
{
  inout_stream << layout.m_uiTotalSize;

  ezUInt16 uiConstants = static_cast<ezUInt16>(layout.m_Constants.GetCount());
  inout_stream << uiConstants;

  for (auto& constant : layout.m_Constants)
  {
    inout_stream << constant.m_sName;
    inout_stream << constant.m_Type;
    inout_stream << constant.m_uiArrayElements;
    inout_stream << constant.m_uiOffset;
  }

  return EZ_SUCCESS;
}

ezResult ezShaderStageBinary::Read(ezStreamReader& inout_stream)
{
  EZ_ASSERT_DEBUG(m_pGALByteCode == nullptr, "");
  m_pGALByteCode = EZ_DEFAULT_NEW(ezGALShaderByteCode);

  ezUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  if (uiVersion < ezShaderStageBinary::Version::Version6)
  {
    ezLog::Error("Old shader binaries are not supported anymore and need to be recompiled, please delete shader cache.");
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiVersion <= ezShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  inout_stream >> m_uiSourceHash;

  // ezGALShaderByteCode
  if (uiVersion >= ezShaderStageBinary::Version::Version7)
  {
    inout_stream >> m_pGALByteCode->m_uiTessellationPatchControlPoints;
  }
  inout_stream >> m_pGALByteCode->m_Stage;
  inout_stream >> m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  {
    ezUInt32 uiByteCodeSize = 0;
    inout_stream >> uiByteCodeSize;
    m_pGALByteCode->m_ByteCode.SetCountUninitialized(uiByteCodeSize);
    if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
      return EZ_FAILURE;
  }

  // m_ShaderResourceBindings
  {
    ezUInt16 uiResources = 0;
    inout_stream >> uiResources;

    m_pGALByteCode->m_ShaderResourceBindings.SetCount(uiResources);

    ezString sTemp;

    for (auto& r : m_pGALByteCode->m_ShaderResourceBindings)
    {
      inout_stream >> r.m_ResourceType;
      inout_stream >> r.m_TextureType;
      inout_stream >> r.m_Stages;
      inout_stream >> r.m_iBindGroup;
      inout_stream >> r.m_iSlot;
      inout_stream >> r.m_uiArraySize;
      inout_stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());

      bool bHasLayout = false;
      inout_stream >> bHasLayout;

      if (bHasLayout)
      {
        r.m_pLayout = EZ_DEFAULT_NEW(ezShaderConstantBufferLayout);
        EZ_SUCCEED_OR_RETURN(Read(inout_stream, *r.m_pLayout));
      }
    }
  }

  // m_ShaderVertexInput
  {
    ezUInt16 uiVertexInputs = 0;
    inout_stream >> uiVertexInputs;
    m_pGALByteCode->m_ShaderVertexInput.SetCount(uiVertexInputs);

    for (auto& v : m_pGALByteCode->m_ShaderVertexInput)
    {
      inout_stream >> v.m_eSemantic;
      inout_stream >> v.m_eFormat;
      inout_stream >> v.m_uiLocation;
    }
  }

  return EZ_SUCCESS;
}



ezResult ezShaderStageBinary::Read(ezStreamReader& inout_stream, ezShaderConstantBufferLayout& out_layout)
{
  inout_stream >> out_layout.m_uiTotalSize;

  ezUInt16 uiConstants = 0;
  inout_stream >> uiConstants;

  out_layout.m_Constants.SetCount(uiConstants);

  for (auto& constant : out_layout.m_Constants)
  {
    inout_stream >> constant.m_sName;
    inout_stream >> constant.m_Type;
    inout_stream >> constant.m_uiArrayElements;
    inout_stream >> constant.m_uiOffset;
  }

  return EZ_SUCCESS;
}

ezSharedPtr<const ezGALShaderByteCode> ezShaderStageBinary::GetByteCode() const
{
  return m_pGALByteCode;
}

ezResult ezShaderStageBinary::WriteStageBinary(ezLogInterface* pLog, ezStringView sPlatform) const
{
  ezStringBuilder sShaderStageFile = ezShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(sPlatform);
  sShaderStageFile.AppendFormat("/{0}_{1}.ezShaderStage", ezGALShaderStage::Names[m_pGALByteCode->m_Stage], ezArgU(m_uiSourceHash, 8, true, 16, true));

  ezFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile).Failed())
  {
    ezLog::Error(pLog, "Could not open shader stage file '{0}' for writing", sShaderStageFile);
    return EZ_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    ezLog::Error(pLog, "Could not write shader stage file '{0}'", sShaderStageFile);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

// static
ezShaderStageBinary* ezShaderStageBinary::LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash, ezStringView sPlatform)
{
  EZ_LOCK(s_ShaderStageBinariesLock);
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    ezStringBuilder sShaderStageFile = ezShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(sPlatform);
    sShaderStageFile.AppendFormat("/{0}_{1}.ezShaderStage", ezGALShaderStage::Names[Stage], ezArgU(uiHash, 8, true, 16, true));

    ezFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      ezLog::Debug("Could not open shader stage file '{0}' for reading", sShaderStageFile);
      return nullptr;
    }

    ezShaderStageBinary shaderStageBinary;
    if (shaderStageBinary.Read(StageFileIn).Failed())
    {
      ezLog::Error("Could not read shader stage file '{0}'", sShaderStageFile);
      return nullptr;
    }

    itStage = ezShaderStageBinary::s_ShaderStageBinaries[Stage].Insert(uiHash, shaderStageBinary);
  }

  ezShaderStageBinary* pShaderStageBinary = &itStage.Value();
  return pShaderStageBinary;
}

// static
void ezShaderStageBinary::OnEngineShutdown()
{
  EZ_LOCK(s_ShaderStageBinariesLock);
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}
