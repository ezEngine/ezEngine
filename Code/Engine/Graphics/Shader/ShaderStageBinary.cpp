#include <Graphics/PCH.h>
#include <Graphics/Shader/ShaderStageBinary.h>
#include <Graphics/ShaderCompiler/ShaderManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezMap<ezUInt32, ezShaderStageBinary> ezShaderStageBinary::s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
ezDeque<ezGALShaderByteCode*> ezShaderStageBinary::s_GALByteCodes;

ezShaderStageBinary::ezShaderStageBinary()
{
  m_uiSourceHash = 0;
  m_Stage = ezGALShaderStage::ENUM_COUNT;
  m_pGALByteCode = nullptr;
}

ezShaderStageBinary::~ezShaderStageBinary()
{
  if (m_pGALByteCode)
  {
    ezGALShaderByteCode* pByteCode = m_pGALByteCode;
    m_pGALByteCode = nullptr;

    //EZ_ASSERT(pByteCode->GetRefCount() == 0, "Shader Bytecode is still referenced.");

    if (pByteCode->GetRefCount() == 0)
      EZ_DEFAULT_DELETE(pByteCode);
  }
}

ezResult ezShaderStageBinary::Write(ezStreamWriterBase& Stream) const
{
  const ezUInt8 uiVersion = 1;

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

  return EZ_SUCCESS;
}

ezResult ezShaderStageBinary::Read(ezStreamReaderBase& Stream)
{
  ezUInt8 uiVersion = 0;

  if (Stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT(uiVersion == 1, "Wrong Version %u", uiVersion);

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

  return EZ_SUCCESS;
}

ezResult ezShaderStageBinary::WriteStageBinary() const
{
  ezStringBuilder sShaderStageFile = ezShaderManager::GetShaderCacheDirectory();

  sShaderStageFile.AppendPath(ezShaderManager::GetPlatform().GetData());
  sShaderStageFile.AppendFormat("/%08X", m_uiSourceHash);

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
    ezStringBuilder sShaderStageFile = ezShaderManager::GetShaderCacheDirectory();

    sShaderStageFile.AppendPath(ezShaderManager::GetPlatform().GetData());
    sShaderStageFile.AppendFormat("/%08X", uiHash);

    ezFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      //ezLog::Error("Could not open shader stage file '%s' for reading", sShaderStageFile.GetData());
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
    pBin->m_pGALByteCode = EZ_DEFAULT_NEW(ezGALShaderByteCode)(&pBin->m_ByteCode[0], pBin->m_ByteCode.GetCount());

  return pBin;
}



