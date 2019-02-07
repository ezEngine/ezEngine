#include <TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureGroupDesc.h>

ezTextureGroupDesc::ezTextureGroupDesc() {}

ezTextureGroupDesc::~ezTextureGroupDesc() {}

ezResult ezTextureGroupDesc::Save(const char* szFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  const ezUInt8 uiVersion = 1;
  file << uiVersion;

  file << m_Groups.GetCount();

  for (ezUInt32 g = 0; g < m_Groups.GetCount(); ++g)
  {
    const auto& group = m_Groups[g];

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      file << group.m_sFilepaths[i];
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureGroupDesc::Load(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezUInt8 uiVersion = 0;
  file >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid texture group desc file version {0}", uiVersion);

  ezUInt32 count = 0;
  file >> count;
  m_Groups.SetCount(count);

  ezStringBuilder tmp;

  for (ezUInt32 g = 0; g < m_Groups.GetCount(); ++g)
  {
    auto& group = m_Groups[g];

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      file >> tmp;
      group.m_sFilepaths[i] = tmp;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasLayerDesc::Serialize(ezStreamWriter& stream) const
{
  stream << m_Usage;
  return stream.WriteArray(m_Filenames);
}

ezResult ezTextureAtlasLayerDesc::Deserialize(ezStreamReader& stream)
{
  stream >> m_Usage;
  return stream.ReadArray(m_Filenames);
}

ezResult ezTextureAtlasDesc::Save(const char* szFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  file.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(file.WriteArray(m_Layers));

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasDesc::Load(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  const ezTypeVersion uiVersion = file.ReadVersion(1);
  EZ_ASSERT_DEV(uiVersion == 1, "Invalid texture atlas desc file version {0}", uiVersion);

  EZ_SUCCEED_OR_RETURN(file.ReadArray(m_Layers));

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TextureGroupDesc);


