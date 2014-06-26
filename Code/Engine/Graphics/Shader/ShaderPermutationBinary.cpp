#include <Graphics/PCH.h>
#include <Graphics/Shader/ShaderPermutationBinary.h>


ezShaderPermutationBinary::ezShaderPermutationBinary()
{
  m_uiShaderStateHash = 0;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

ezResult ezShaderPermutationBinary::Write(ezStreamWriterBase& Stream) const
{
  const ezUInt8 uiVersion = 1;

  if (Stream.WriteBytes(&uiVersion, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  if (Stream.WriteDWordValue(&m_uiShaderStateHash).Failed())
    return EZ_FAILURE;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (Stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezShaderPermutationBinary::Read(ezStreamReaderBase& Stream)
{
  ezUInt8 uiVersion = 0;

  if (Stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT(uiVersion == 1, "Wrong Version %u", uiVersion);

  if (Stream.ReadDWordValue(&m_uiShaderStateHash).Failed())
    return EZ_FAILURE;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (Stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}


