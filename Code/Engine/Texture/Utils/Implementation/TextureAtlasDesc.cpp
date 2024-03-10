#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

ezResult ezTextureAtlasCreationDesc::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return EZ_FAILURE;

  const ezUInt8 uiNumLayers = static_cast<ezUInt8>(m_Layers.GetCount());
  inout_stream << uiNumLayers;

  for (ezUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream << m_Layers[l].m_Usage;
    inout_stream << m_Layers[l].m_uiNumChannels;
  }

  inout_stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    inout_stream << item.m_uiUniqueID;
    inout_stream << item.m_uiFlags;

    for (ezUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream << item.m_sLayerInput[l];
    }

    inout_stream << item.m_sAlphaInput;
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasCreationDesc::Deserialize(ezStreamReader& inout_stream)
{
  const ezTypeVersion uiVersion = inout_stream.ReadVersion(3);

  ezUInt8 uiNumLayers = 0;
  inout_stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (ezUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream >> m_Layers[l].m_Usage;
    inout_stream >> m_Layers[l].m_uiNumChannels;
  }

  ezUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    inout_stream >> item.m_uiUniqueID;
    inout_stream >> item.m_uiFlags;

    for (ezUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      inout_stream >> item.m_sAlphaInput;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasCreationDesc::Save(ezStringView sFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  return Serialize(file);
}

ezResult ezTextureAtlasCreationDesc::Load(ezStringView sFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  return Deserialize(file);
}

void ezTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

ezResult ezTextureAtlasRuntimeDesc::Serialize(ezStreamWriter& inout_stream) const
{
  m_Items.Sort();

  inout_stream << m_uiNumLayers;
  inout_stream << m_Items.GetCount();

  for (ezUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    inout_stream << m_Items.GetKey(i);
    inout_stream << m_Items.GetValue(i).m_uiFlags;

    for (ezUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      inout_stream << r.x;
      inout_stream << r.y;
      inout_stream << r.width;
      inout_stream << r.height;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTextureAtlasRuntimeDesc::Deserialize(ezStreamReader& inout_stream)
{
  Clear();

  inout_stream >> m_uiNumLayers;

  ezUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (ezUInt32 i = 0; i < uiNumItems; ++i)
  {
    ezUInt32 key = 0;
    inout_stream >> key;

    auto& item = m_Items[key];
    inout_stream >> item.m_uiFlags;

    for (ezUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      inout_stream >> r.x;
      inout_stream >> r.y;
      inout_stream >> r.width;
      inout_stream >> r.height;
    }
  }

  m_Items.Sort();
  return EZ_SUCCESS;
}
