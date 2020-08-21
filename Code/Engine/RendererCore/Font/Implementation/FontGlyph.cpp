#include <RendererCorePCH.h>

#include <RendererCore/Font/FontGlyph.h>

ezResult ezKerningPair::Serialize(ezStreamWriter& stream) const
{
  stream << m_OtherCharacterId;
  stream << m_Amount;

  return EZ_SUCCESS;
}

ezResult ezKerningPair::Deserialize(ezStreamReader& stream)
{
  stream >> m_OtherCharacterId;
  stream >> m_Amount;
  return EZ_SUCCESS;
}

ezResult ezFontGlyph::Serialize(ezStreamWriter& stream) const
{
  stream << m_CharacterId;
  stream << m_Page;
  stream << m_UVX;
  stream << m_UVY;
  stream << m_UVWidth;
  stream << m_UVHeight;
  stream << m_Width;
  stream << m_Height;
  stream << m_XOffset;
  stream << m_YOffset;
  stream << m_XAdvance;
  stream << m_YAdvance;
  stream.WriteArray(m_KerningPairs);

  return EZ_SUCCESS;
}

ezResult ezFontGlyph::Deserialize(ezStreamReader& stream)
{
  stream >> m_CharacterId;
  stream >> m_Page;
  stream >> m_UVX;
  stream >> m_UVY;
  stream >> m_UVWidth;
  stream >> m_UVHeight;
  stream >> m_Width;
  stream >> m_Height;
  stream >> m_XOffset;
  stream >> m_YOffset;
  stream >> m_XAdvance;
  stream >> m_YAdvance;
  stream.ReadArray(m_KerningPairs);

  return EZ_SUCCESS;
}
