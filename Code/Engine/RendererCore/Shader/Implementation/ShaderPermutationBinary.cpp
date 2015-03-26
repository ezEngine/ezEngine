#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <Foundation/Logging/Log.h>

enum ezShaderPermutationBinaryVersion
{
  Version1 = 1,

  ENUM_COUNT,
  Current = ENUM_COUNT - 1
};

ezShaderPermutationBinary::ezShaderPermutationBinary()
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

ezResult ezShaderPermutationBinary::Write(ezStreamWriterBase& Stream)
{
  // write this at the beginning so that the file can be read as an ezDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  m_DependencyFile.WriteDependencyFile(Stream);

  const ezUInt8 uiVersion = ezShaderPermutationBinaryVersion::Current;

  if (Stream.WriteBytes(&uiVersion, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (Stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  m_StateDescriptor.Save(Stream);

  return EZ_SUCCESS;
}

ezResult ezShaderPermutationBinary::Read(ezStreamReaderBase& Stream)
{
  m_DependencyFile.ReadDependencyFile(Stream);

  ezUInt8 uiVersion = 0;

  if (Stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT_DEV(uiVersion <= ezShaderPermutationBinaryVersion::Current, "Wrong Version %u", uiVersion);

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (Stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  m_StateDescriptor.Load(Stream);

  return EZ_SUCCESS;
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_ShaderPermutationBinary);

