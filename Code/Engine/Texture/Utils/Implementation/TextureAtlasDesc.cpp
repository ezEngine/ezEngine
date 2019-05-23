#include <TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

ezResult ezTextureAtlasCreationDesc::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  if(m_Layers.GetCount() > 255u)
    return EZ_FAILURE;

  const ezUInt8 uiNumLayers = static_cast<ezUInt8>(m_Layers.GetCount());
  stream << uiNumLayers;

  for (ezUInt32 l = 0; l < uiNumLayers; ++l)
  {
    stream << m_Layers[l].m_Usage;
    stream << m_Layers[l].m_uiNumChannels;
  }

  stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    stream << item.m_uiUniqueID;

    for (ezUInt32 l = 0; l < uiNumLayers; ++l)
    {
      stream << item.m_sLayerInput[l];
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasCreationDesc::Deserialize(ezStreamReader& stream)
{
  const ezTypeVersion uiVersion = stream.ReadVersion(1);
  EZ_ASSERT_DEV(uiVersion == 1, "Invalid texture atlas desc file version {0}", uiVersion);

  ezUInt8 uiNumLayers = 0;
  stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (ezUInt32 l = 0; l < uiNumLayers; ++l)
  {
    stream >> m_Layers[l].m_Usage;
    stream >> m_Layers[l].m_uiNumChannels;
  }

  ezUInt32 uiNumItems = 0;
  stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    stream >> item.m_uiUniqueID;

    for (ezUInt32 l = 0; l < uiNumLayers; ++l)
    {
      stream >> item.m_sLayerInput[l];
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasCreationDesc::Save(const char* szFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  return Serialize(file);
}

ezResult ezTextureAtlasCreationDesc::Load(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  return Deserialize(file);
}

void ezTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

ezResult ezTextureAtlasRuntimeDesc::Serialize(ezStreamWriter& stream) const
{
  m_Items.Sort();

  stream << m_uiNumLayers;
  stream << m_Items.GetCount();

  for (ezUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    stream << m_Items.GetKey(i);

    for (ezUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      stream << r.x;
      stream << r.y;
      stream << r.width;
      stream << r.height;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasRuntimeDesc::Deserialize(ezStreamReader& stream)
{
  Clear();

  stream >> m_uiNumLayers;

  ezUInt32 uiNumItems = 0;
  stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (ezUInt32 i = 0; i < uiNumItems; ++i)
  {
    ezUInt32 key = 0;
    stream >> key;

    auto& item = m_Items[key];

    for (ezUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      stream >> r.x;
      stream >> r.y;
      stream >> r.width;
      stream >> r.height;
    }
  }

  m_Items.Sort();
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TextureAtlasDesc);
