#include <Graphics/PCH.h>
#include <Graphics/Shader/ShaderStageBinary.h>

ezMap<ezUInt32, ezShaderStageBinary> ezShaderStageBinary::s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];

ezShaderStageBinary::ezShaderStageBinary()
{
  m_uiSourceHash = 0;
  m_Stage = ezGALShaderStage::ENUM_COUNT;
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





