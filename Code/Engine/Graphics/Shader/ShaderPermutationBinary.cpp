#include <Graphics/PCH.h>
#include <Graphics/Shader/ShaderPermutationBinary.h>

enum ezShaderPermutationBinaryVersion
{
  Version1 = 1,
  Version2,
  Version3,

  ENUM_COUNT,
  Current = ENUM_COUNT - 1
};

ezShaderPermutationBinary::ezShaderPermutationBinary()
{
  m_uiShaderStateHash = 0;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

ezResult ezShaderPermutationBinary::Write(ezStreamWriterBase& Stream) const
{
  const ezUInt8 uiVersion = ezShaderPermutationBinaryVersion::Current;

  if (Stream.WriteBytes(&uiVersion, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  if (Stream.WriteDWordValue(&m_uiShaderStateHash).Failed())
    return EZ_FAILURE;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (Stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  // Version 2

  Stream << m_IncludeFiles.GetCount();

  for (ezUInt32 i = 0; i < m_IncludeFiles.GetCount(); ++i)
  {
    Stream << m_IncludeFiles[i];
  }

  Stream << m_iMaxTimeStamp;

  return EZ_SUCCESS;
}

ezResult ezShaderPermutationBinary::Read(ezStreamReaderBase& Stream)
{
  ezUInt8 uiVersion = 0;

  if (Stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT(uiVersion <= ezShaderPermutationBinaryVersion::Current, "Wrong Version %u", uiVersion);

  if (Stream.ReadDWordValue(&m_uiShaderStateHash).Failed())
    return EZ_FAILURE;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (Stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  if (uiVersion >= ezShaderPermutationBinaryVersion::Version2)
  {
    ezUInt32 uiIncludes = 0;
    Stream >> uiIncludes;
    m_IncludeFiles.SetCount(uiIncludes);

    for (ezUInt32 i = 0; i < m_IncludeFiles.GetCount(); ++i)
    {
      Stream >> m_IncludeFiles[i];
    }
  }

  Stream >> m_iMaxTimeStamp;

  // if this is an older version, make sure the dependent files check will always return that something changed
  if (uiVersion < ezShaderPermutationBinaryVersion::Current)
    m_iMaxTimeStamp = 0;

  return EZ_SUCCESS;
}


