#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct ezShaderPermutationBinaryVersion
{
  enum Enum : ezUInt32
  {
    Version1 = 1,
    Version2 = 2,
    Version3 = 3,
    Version4 = 4,
    Version5 = 5,
    Version6 = 6, // Fixed DX11 particles vanishing

    // Increase this version number to trigger shader recompilation

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

ezShaderPermutationBinary::ezShaderPermutationBinary()
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

ezResult ezShaderPermutationBinary::Write(ezStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an ezDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  EZ_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  const ezUInt8 uiVersion = ezShaderPermutationBinaryVersion::Current;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(ezUInt8)).Failed())
    return EZ_FAILURE;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  m_StateDescriptor.Save(inout_stream);

  inout_stream << m_PermutationVars.GetCount();

  for (auto& var : m_PermutationVars)
  {
    inout_stream << var.m_sName.GetString();
    inout_stream << var.m_sValue.GetString();
  }

  return EZ_SUCCESS;
}

ezResult ezShaderPermutationBinary::Read(ezStreamReader& inout_stream, bool& out_bOldVersion)
{
  EZ_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  ezUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(ezUInt8)) != sizeof(ezUInt8))
    return EZ_FAILURE;

  EZ_ASSERT_DEV(uiVersion <= ezShaderPermutationBinaryVersion::Current, "Wrong Version {0}", uiVersion);

  out_bOldVersion = uiVersion != ezShaderPermutationBinaryVersion::Current;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return EZ_FAILURE;
  }

  m_StateDescriptor.Load(inout_stream);

  if (uiVersion >= ezShaderPermutationBinaryVersion::Version2)
  {
    ezUInt32 uiPermutationCount;
    inout_stream >> uiPermutationCount;

    m_PermutationVars.SetCount(uiPermutationCount);

    ezStringBuilder tmp;
    for (ezUInt32 i = 0; i < uiPermutationCount; ++i)
    {
      auto& var = m_PermutationVars[i];

      inout_stream >> tmp;
      var.m_sName.Assign(tmp.GetData());
      inout_stream >> tmp;
      var.m_sValue.Assign(tmp.GetData());
    }
  }

  return EZ_SUCCESS;
}
