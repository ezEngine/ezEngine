#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezMap<ezUInt32, ezShaderStageBinary> ezShaderStageBinary::s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
ezMap<ezUInt32, ezShaderMaterialParamCB> ezShaderStageBinary::s_ShaderMaterialParamCBs;

ezShaderMaterialParamCB::ezShaderMaterialParamCB()
{
  m_uiMaterialCBSize = 0;
  m_uiLastBufferModification = 0;
}

ezUInt32 ezShaderMaterialParamCB::GetHash() const
{
  ezStringBuilder s;
  s.AppendFormat("%u/%u", m_uiMaterialCBSize, m_MaterialParameters.GetCount());

  for (const auto& v : m_MaterialParameters)
  {
    s.AppendFormat("_%u/%u/%u", v.m_uiOffset, v.m_uiNameHash, ezShaderMaterialParamCB::MaterialParameter::s_TypeSize[(ezUInt32) v.m_Type] * v.m_uiArrayElements);
  }

  return ezHashing::MurmurHash(s.GetData());
}

ezShaderStageBinary::ezShaderStageBinary()
{
  m_uiSourceHash = 0;
  m_Stage = ezGALShaderStage::ENUM_COUNT;
  m_pGALByteCode = nullptr;
  m_pMaterialParamCB = nullptr;
}

ezShaderStageBinary::~ezShaderStageBinary()
{
  if (m_pGALByteCode)
  {
    ezGALShaderByteCode* pByteCode = m_pGALByteCode;
    m_pGALByteCode = nullptr;

    if (pByteCode->GetRefCount() == 0)
      EZ_DEFAULT_DELETE(pByteCode);
  }
}

void ezShaderStageBinary::OnEngineShutdown()
{
  s_ShaderMaterialParamCBs.Clear();

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    s_ShaderStageBinaries[stage].Clear();
}

ezResult ezShaderStageBinary::Write(ezStreamWriterBase& Stream) const
{
  const ezUInt8 uiVersion = ezShaderStageBinary::VersionCurrent;

  if (Stream.WriteBytes(&uiVersion, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  if (Stream.WriteDWordValue(&m_uiSourceHash).Failed())
    return EZ_FAILURE;

  const ezUInt8 uiStage = (ezUInt8) m_Stage;

  if (Stream.WriteBytes(&uiStage, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  const ezUInt32 uiByteCodeSize = m_ByteCode.GetCount();

  if (Stream.WriteDWordValue(&uiByteCodeSize).Failed())
    return EZ_FAILURE;

  if (!m_ByteCode.IsEmpty() && Stream.WriteBytes(&m_ByteCode[0], uiByteCodeSize).Failed())
    return EZ_FAILURE;

  ezUInt16 uiResources = m_ShaderResourceBindings.GetCount();
  Stream << uiResources;

  for (const ezShaderStageResource& r : m_ShaderResourceBindings)
  {
    Stream << r.m_Name.GetData();
    Stream << r.m_iSlot;
    Stream << (ezUInt8) r.m_Type;
  }

  if (m_pMaterialParamCB)
  {
    Stream << m_pMaterialParamCB->m_uiMaterialCBSize;

    ezUInt16 uiMaterialParams = m_pMaterialParamCB->m_MaterialParameters.GetCount();
    Stream << uiMaterialParams;

    for (const auto& mp : m_pMaterialParamCB->m_MaterialParameters)
    {
      Stream << mp.m_uiNameHash;
      Stream << (ezUInt8) mp.m_Type;
      Stream << mp.m_uiArrayElements;
      Stream << mp.m_uiOffset;
    }
  }
  else
  {
    ezUInt32 uiMaterialCBSize = 0;
    Stream << uiMaterialCBSize;

    ezUInt16 uiMaterialParams = 0;
    Stream << uiMaterialParams;
  }

  return EZ_SUCCESS;
}

ezResult ezShaderStageBinary::Read(ezStreamReaderBase& Stream)
{
  ezUInt8 uiVersion = 0;

  if (Stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT_DEV(uiVersion <= ezShaderStageBinary::VersionCurrent, "Wrong Version %u", uiVersion);

  if (Stream.ReadDWordValue(&m_uiSourceHash).Failed())
    return EZ_FAILURE;

  ezUInt8 uiStage = ezGALShaderStage::ENUM_COUNT;

  if (Stream.ReadBytes(&uiStage, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  m_Stage = (ezGALShaderStage::Enum) uiStage;

  ezUInt32 uiByteCodeSize = 0;

  if (Stream.ReadDWordValue(&uiByteCodeSize).Failed())
    return EZ_FAILURE;

  m_ByteCode.SetCount(uiByteCodeSize);

  if (!m_ByteCode.IsEmpty() && Stream.ReadBytes(&m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
    return EZ_FAILURE;

  if (uiVersion >= ezShaderStageBinary::Version2)
  {
    ezUInt16 uiResources = 0;
    Stream >> uiResources;

    m_ShaderResourceBindings.SetCount(uiResources);

    ezString sTemp;

    for (ezShaderStageResource& r : m_ShaderResourceBindings)
    {
      Stream >> sTemp;
      r.m_Name.Assign(sTemp.GetData());
      Stream >> r.m_iSlot;

      ezUInt8 uiType = 0;
      Stream >> uiType;
      r.m_Type = (ezShaderStageResource::ResourceType) uiType;
    }
  }

  if (uiVersion >= ezShaderStageBinary::Version3)
  {
    ezShaderMaterialParamCB mcb;

    Stream >> mcb.m_uiMaterialCBSize;

    ezUInt16 uiParameters = 0;
    Stream >> uiParameters;

    mcb.m_MaterialParameters.SetCount(uiParameters);

    ezUInt8 uiTemp8;

    for (auto& mp : mcb.m_MaterialParameters)
    {
      Stream >> mp.m_uiNameHash;
      Stream >> uiTemp8; mp.m_Type = (ezShaderMaterialParamCB::MaterialParameter::Type) uiTemp8;
      Stream >> mp.m_uiArrayElements;
      Stream >> mp.m_uiOffset;
    }

    CreateMaterialParamObject(mcb);
  }

  return EZ_SUCCESS;
}

void ezShaderStageBinary::CreateMaterialParamObject(const ezShaderMaterialParamCB& matparams)
{
  ezUInt32 uiHash = matparams.GetHash();

  bool bExisted = false;
  auto it = s_ShaderMaterialParamCBs.FindOrAdd(uiHash, &bExisted);

  if (!bExisted)
  {
    it.Value() = matparams;
  }

  m_pMaterialParamCB = &it.Value();
}

ezResult ezShaderStageBinary::WriteStageBinary() const
{
  ezStringBuilder sShaderStageFile = ezRenderContext::GetShaderCacheDirectory();

  sShaderStageFile.AppendPath(ezRenderContext::GetActiveShaderPlatform().GetData());
  sShaderStageFile.AppendFormat("/%08X.ezShaderStage", m_uiSourceHash);

  ezFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile.GetData()).Failed())
  {
    ezLog::Error("Could not open shader stage file '%s' for writing", sShaderStageFile.GetData());
    return EZ_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    ezLog::Error("Could not write shader stage file '%s'", sShaderStageFile.GetData());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezShaderStageBinary* ezShaderStageBinary::LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash)
{
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    ezStringBuilder sShaderStageFile = ezRenderContext::GetShaderCacheDirectory();

    sShaderStageFile.AppendPath(ezRenderContext::GetActiveShaderPlatform().GetData());
    sShaderStageFile.AppendFormat("/%08X.ezShaderStage", uiHash);

    ezFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      ezLog::Debug("Could not open shader stage file '%s' for reading", sShaderStageFile.GetData());
      return nullptr;
    }

    ezShaderStageBinary ssb;
    if (ssb.Read(StageFileIn).Failed())
    {
      ezLog::Error("Could not read shader stage file '%s'", sShaderStageFile.GetData());
      return nullptr;
    }

    itStage = ezShaderStageBinary::s_ShaderStageBinaries[Stage].Insert(uiHash, ssb);
  }

  if (!itStage.IsValid())
    return nullptr;

  ezShaderStageBinary* pBin = &itStage.Value();

  if (pBin->m_pGALByteCode == nullptr && !pBin->m_ByteCode.IsEmpty())
    pBin->m_pGALByteCode = EZ_DEFAULT_NEW(ezGALShaderByteCode, &pBin->m_ByteCode[0], pBin->m_ByteCode.GetCount());

  return pBin;
}





EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_ShaderStageBinary);

