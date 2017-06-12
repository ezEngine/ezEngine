#include <PCH.h>
#include <RendererCore/Textures/TexturePacker.h>

ezTexturePacker::ezTexturePacker()
{

}

ezTexturePacker::~ezTexturePacker()
{

}

void ezTexturePacker::SetTextureSize(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiReserveTextures /*= 0*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  m_Textures.Clear();
  m_Textures.Reserve(uiReserveTextures);

  // initializes all values to false
  m_Grid.Clear();
  m_Grid.SetCount(m_uiWidth * m_uiHeight);
}

void ezTexturePacker::AddTexture(ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  Texture& tex = m_Textures.ExpandAndGetRef();
  tex.m_Size.x = uiWidth;
  tex.m_Size.y = uiHeight;
  tex.m_Priority = 2 * uiWidth + 2 * uiHeight;
}

ezResult ezTexturePacker::PackTextures()
{
  m_Textures.Sort([](const Texture& lhs, const Texture& rhs) -> bool
  {
    return lhs.m_Priority > rhs.m_Priority;
  });

  for (ezUInt32 idx = 0; idx < m_Textures.GetCount(); ++idx)
  {
    if (!TryPlaceTexture(idx))
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezUInt32 ezTexturePacker::PosToIndex(ezUInt32 x, ezUInt32 y) const
{
  return (y * m_uiWidth + x);
}

bool ezTexturePacker::TryPlaceTexture(ezUInt32 idx)
{
  Texture& tex = m_Textures[idx];

  for (ezUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (ezUInt32 x = 0; x < m_uiWidth; ++x)
    {
      if (!TryPlaceAt(ezVec2U32(x, y), tex.m_Size))
          continue;

      tex.m_Position.Set(x, y);
      return true;
    }
  }

  return false;
}

bool ezTexturePacker::CanPlaceAt(ezVec2U32 pos, ezVec2U32 size)
{
  if (pos.x + size.x > m_uiWidth)
    return false;
  if (pos.y + size.y > m_uiHeight)
    return false;

  for (ezUInt32 y = 0; y < size.y; ++y)
  {
    for (ezUInt32 x = 0; x < size.x; ++x)
    {
      if (m_Grid[PosToIndex(pos.x + x, pos.y + y)])
        return false;
    }
  }

  return true;
}

bool ezTexturePacker::TryPlaceAt(ezVec2U32 pos, ezVec2U32 size)
{
  if (!CanPlaceAt(pos, size))
    return false;

  for (ezUInt32 y = 0; y < size.y; ++y)
  {
    for (ezUInt32 x = 0; x < size.x; ++x)
    {
      m_Grid[PosToIndex(pos.x + x, pos.y + y)] = true;
    }
  }

  return true;
}