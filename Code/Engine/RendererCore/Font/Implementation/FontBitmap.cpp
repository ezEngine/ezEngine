#include <RendererCorePCH.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <RendererCore/Font/FontBitmap.h>

const ezFontGlyph& ezFontBitmap::GetFontGlyph(ezUInt32 characterId) const
{
  auto it = m_Characters.Find(characterId);
  if (it.IsValid())
  {
    return it.Value();
  }

  return m_MissingGlyph;
}

ezResult ezRawFontBitmap::Serialize(ezStreamWriter& stream) const
{
  stream << m_Size;
  stream << m_BaselineOffset;
  stream << m_LineHeight;
  m_MissingGlyph.Serialize(stream);
  stream << m_SpaceWidth;

  ezUInt32 texturesCount = m_Textures.GetCount();

  stream << texturesCount;

  ezDdsFileFormat ddsFileFormat;

  for (ezUInt32 i = 0; i < texturesCount; i++)
  {
    if (ddsFileFormat.WriteImage(stream, m_Textures[i], ezLog::GetThreadLocalLogSystem(), "dds").Failed())
    {
      ezLog::Error("Failed to write DDS image chunk to ezFont file.");
      return EZ_FAILURE;
    }
  }

  stream.WriteMap(m_Characters);

  return EZ_SUCCESS;
}

ezResult ezRawFontBitmap::Deserialize(ezStreamReader& stream)
{
  stream >> m_Size;
  stream >> m_BaselineOffset;
  stream >> m_LineHeight;
  m_MissingGlyph.Deserialize(stream);
  stream >> m_SpaceWidth;

  ezUInt32 texturesCount = 0;
  stream >> texturesCount;

  m_Textures.Clear();
  m_Textures.Reserve(texturesCount);

  ezDdsFileFormat ddsFileFormat;
  for (ezUInt32 i = 0; i < texturesCount; i++)
  {
    ezImage& img = m_Textures.ExpandAndGetRef();
    if(ddsFileFormat.ReadImage(stream, img, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
    {
      ezLog::Error("Failed to read DDS image chunk from ezFont file.");
      return EZ_FAILURE;
    }
  }

  stream.ReadMap(m_Characters);

  return EZ_SUCCESS;
}
