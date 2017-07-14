#include <PCH.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/Shader/Types.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezUInt32 ezShaderConstantBufferLayout::Constant::s_TypeSize[(ezUInt32)Type::ENUM_COUNT] =
{
  0,
  sizeof(float) * 1,
  sizeof(float) * 2,
  sizeof(float) * 3,
  sizeof(float) * 4,
  sizeof(int) * 1,
  sizeof(int) * 2,
  sizeof(int) * 3,
  sizeof(int) * 4,
  sizeof(ezUInt32) * 1,
  sizeof(ezUInt32) * 2,
  sizeof(ezUInt32) * 3,
  sizeof(ezUInt32) * 4,
  sizeof(ezShaderMat3),
  sizeof(ezMat4),
  sizeof(ezShaderTransform),
  sizeof(ezShaderBool)
};

void ezShaderConstantBufferLayout::Constant::CopyDataFormVariant(ezUInt8* pDest, ezVariant* pValue) const
{
  EZ_ASSERT_DEV(m_uiArrayElements == 1, "Array constants are not supported");

  ezResult conversionResult = EZ_FAILURE;

  if (pValue != nullptr)
  {
    switch (m_Type)
    {
    case Type::Float1:
      *reinterpret_cast<float*>(pDest) = pValue->ConvertTo<float>(&conversionResult); break;
    case Type::Float2:
      *reinterpret_cast<ezVec2*>(pDest) = pValue->Get<ezVec2>(); return;
    case Type::Float3:
      *reinterpret_cast<ezVec3*>(pDest) = pValue->Get<ezVec3>(); return;
    case Type::Float4:
      if (pValue->GetType() == ezVariant::Type::Color || pValue->GetType() == ezVariant::Type::ColorGamma)
      {
        const ezColor tmp = pValue->ConvertTo<ezColor>();
        *reinterpret_cast<ezVec4*>(pDest) = *reinterpret_cast<const ezVec4*>(&tmp);
      }
      else
      {
        *reinterpret_cast<ezVec4*>(pDest) = pValue->Get<ezVec4>();
      }
      return;

    case Type::Int1:
      *reinterpret_cast<ezInt32*>(pDest) = pValue->ConvertTo<ezInt32>(&conversionResult); break;
    case Type::Int2:
      *reinterpret_cast<ezVec2I32*>(pDest) = pValue->Get<ezVec2I32>(); return;
    case Type::Int3:
      *reinterpret_cast<ezVec3I32*>(pDest) = pValue->Get<ezVec3I32>(); return;
    case Type::Int4:
      *reinterpret_cast<ezVec4I32*>(pDest) = pValue->Get<ezVec4I32>(); return;

    case Type::UInt1:
      *reinterpret_cast<ezUInt32*>(pDest) = pValue->ConvertTo<ezUInt32>(&conversionResult); break;
    case Type::UInt2:
      *reinterpret_cast<ezVec2U32*>(pDest) = pValue->Get<ezVec2U32>(); return;
    case Type::UInt3:
      *reinterpret_cast<ezVec3U32*>(pDest) = pValue->Get<ezVec3U32>(); return;
    case Type::UInt4:
      *reinterpret_cast<ezVec4U32*>(pDest) = pValue->Get<ezVec4U32>(); return;

    case Type::Mat3x3:
      *reinterpret_cast<ezShaderMat3*>(pDest) = pValue->Get<ezMat3>(); return;
    case Type::Mat4x4:
      *reinterpret_cast<ezMat4*>(pDest) = pValue->Get<ezMat4>(); return;
    case Type::Transform:
      *reinterpret_cast<ezShaderTransform*>(pDest) = pValue->Get<ezTransform>(); return;

    case Type::Bool:
      *reinterpret_cast<ezShaderBool*>(pDest) = pValue->ConvertTo<bool>(&conversionResult); break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (conversionResult.Succeeded())
  {
    return;
  }

  //ezLog::Error("Constant '{0}' is not set, invalid or couldn't be converted to target type and will be set to zero.", m_sName);
  const ezUInt32 uiSize = s_TypeSize[m_Type];
  ezMemoryUtils::ZeroFill(pDest, uiSize);
}

ezShaderConstantBufferLayout::ezShaderConstantBufferLayout()
{
  m_uiTotalSize = 0;
}

ezShaderConstantBufferLayout::~ezShaderConstantBufferLayout()
{
}

ezResult ezShaderConstantBufferLayout::Write(ezStreamWriter& stream) const
{
  stream << m_uiTotalSize;

  ezUInt16 uiConstants = m_Constants.GetCount();
  stream << uiConstants;

  for (auto& constant : m_Constants)
  {
    stream << constant.m_sName;
    stream << constant.m_Type;
    stream << constant.m_uiArrayElements;
    stream << constant.m_uiOffset;
  }

  return EZ_SUCCESS;
}

ezResult ezShaderConstantBufferLayout::Read(ezStreamReader& stream)
{
  stream >> m_uiTotalSize;

  ezUInt16 uiConstants = 0;
  stream >> uiConstants;

  m_Constants.SetCount(uiConstants);

  for (auto& constant : m_Constants)
  {
    stream >> constant.m_sName;
    stream >> constant.m_Type;
    stream >> constant.m_uiArrayElements;
    stream >> constant.m_uiOffset;
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezShaderResourceBinding::ezShaderResourceBinding()
{
  m_Type = Unknown;
  m_iSlot = -1;
  m_pLayout = nullptr;
}

ezShaderResourceBinding::~ezShaderResourceBinding()
{

}

//////////////////////////////////////////////////////////////////////////

ezMap<ezUInt32, ezShaderStageBinary> ezShaderStageBinary::s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];

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

    if (pByteCode->GetRefCount() == 0)
      EZ_DEFAULT_DELETE(pByteCode);
  }

  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_pLayout != nullptr)
    {
      ezShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      binding.m_pLayout = nullptr;

      if (pLayout->GetRefCount() == 0)
        EZ_DEFAULT_DELETE(pLayout);
    }
  }
}

ezResult ezShaderStageBinary::Write(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = ezShaderStageBinary::VersionCurrent;

  if (stream.WriteBytes(&uiVersion, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  if (stream.WriteDWordValue(&m_uiSourceHash).Failed())
    return EZ_FAILURE;

  const ezUInt8 uiStage = (ezUInt8) m_Stage;

  if (stream.WriteBytes(&uiStage, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  const ezUInt32 uiByteCodeSize = m_ByteCode.GetCount();

  if (stream.WriteDWordValue(&uiByteCodeSize).Failed())
    return EZ_FAILURE;

  if (!m_ByteCode.IsEmpty() && stream.WriteBytes(&m_ByteCode[0], uiByteCodeSize).Failed())
    return EZ_FAILURE;

  ezUInt16 uiResources = m_ShaderResourceBindings.GetCount();
  stream << uiResources;

  for (const auto& r : m_ShaderResourceBindings)
  {
    stream << r.m_sName.GetData();
    stream << r.m_iSlot;
    stream << (ezUInt8) r.m_Type;

    if (r.m_Type == ezShaderResourceBinding::ConstantBuffer)
    {
      r.m_pLayout->Write(stream);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezShaderStageBinary::Read(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  if (stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT_DEV(uiVersion <= ezShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  if (stream.ReadDWordValue(&m_uiSourceHash).Failed())
    return EZ_FAILURE;

  ezUInt8 uiStage = ezGALShaderStage::ENUM_COUNT;

  if (stream.ReadBytes(&uiStage, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  m_Stage = (ezGALShaderStage::Enum) uiStage;

  ezUInt32 uiByteCodeSize = 0;

  if (stream.ReadDWordValue(&uiByteCodeSize).Failed())
    return EZ_FAILURE;

  m_ByteCode.SetCountUninitialized(uiByteCodeSize);

  if (!m_ByteCode.IsEmpty() && stream.ReadBytes(&m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
    return EZ_FAILURE;

  if (uiVersion >= ezShaderStageBinary::Version2)
  {
    ezUInt16 uiResources = 0;
    stream >> uiResources;

    m_ShaderResourceBindings.SetCount(uiResources);

    ezString sTemp;

    for (auto& r : m_ShaderResourceBindings)
    {
      stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());
      stream >> r.m_iSlot;

      ezUInt8 uiType = 0;
      stream >> uiType;
      r.m_Type = (ezShaderResourceBinding::ResourceType) uiType;

      if (r.m_Type == ezShaderResourceBinding::ConstantBuffer && uiVersion >= ezShaderStageBinary::Version4)
      {
        auto pLayout = EZ_DEFAULT_NEW(ezShaderConstantBufferLayout);
        pLayout->Read(stream);

        r.m_pLayout = pLayout;
      }
    }
  }

  return EZ_SUCCESS;
}


ezDynamicArray<ezUInt8>& ezShaderStageBinary::GetByteCode()
{
  return m_ByteCode;
}

void ezShaderStageBinary::AddShaderResourceBinding(const ezShaderResourceBinding& binding)
{
  m_ShaderResourceBindings.PushBack(binding);
}


ezArrayPtr<const ezShaderResourceBinding> ezShaderStageBinary::GetShaderResourceBindings() const
{
  return m_ShaderResourceBindings;
}

const ezShaderResourceBinding* ezShaderStageBinary::GetShaderResourceBinding(const ezTempHashedString& sName) const
{
  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }

  return nullptr;
}

ezShaderConstantBufferLayout* ezShaderStageBinary::CreateConstantBufferLayout() const
{
  return EZ_DEFAULT_NEW(ezShaderConstantBufferLayout);
}

ezResult ezShaderStageBinary::WriteStageBinary(ezLogInterface* pLog) const
{
  ezStringBuilder sShaderStageFile = ezShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(ezShaderManager::GetActivePlatform().GetData());
  sShaderStageFile.AppendFormat("/{0}{1}.ezShaderStage", ezGALShaderStage::Names[m_Stage], ezArgU(m_uiSourceHash, 8, true, 16, true));

  ezFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile.GetData()).Failed())
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

//static
ezShaderStageBinary* ezShaderStageBinary::LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash)
{
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    ezStringBuilder sShaderStageFile = ezShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(ezShaderManager::GetActivePlatform().GetData());
    sShaderStageFile.AppendFormat("/{0}{1}.ezShaderStage", ezGALShaderStage::Names[Stage], ezArgU(uiHash, 8, true, 16, true));

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

  if (!itStage.IsValid())
  {
    return nullptr;
  }

  ezShaderStageBinary* pShaderStageBinary = &itStage.Value();

  if (pShaderStageBinary->m_pGALByteCode == nullptr && !pShaderStageBinary->m_ByteCode.IsEmpty())
  {
    pShaderStageBinary->m_pGALByteCode = EZ_DEFAULT_NEW(ezGALShaderByteCode, pShaderStageBinary->m_ByteCode);
  }

  return pShaderStageBinary;
}

//static
void ezShaderStageBinary::OnEngineShutdown()
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderStageBinary);

