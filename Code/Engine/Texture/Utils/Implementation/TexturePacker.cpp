#include <PCH.h>

#include <Texture/Utils/TexturePacker.h>

ezTexturePacker::ezTexturePacker() {}

ezTexturePacker::~ezTexturePacker() {}

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

struct sortdata
{
  EZ_DECLARE_POD_TYPE();

  ezInt32 m_Priority;
  ezInt32 m_Index;
};

ezResult ezTexturePacker::PackTextures()
{
  ezDynamicArray<sortdata> sorted;
  sorted.SetCountUninitialized(m_Textures.GetCount());

  for (ezUInt32 i = 0; i < m_Textures.GetCount(); ++i)
  {
    sorted[i].m_Index = i;
    sorted[i].m_Priority = m_Textures[i].m_Priority;
  }

  sorted.Sort([](const sortdata& lhs, const sortdata& rhs) -> bool { return lhs.m_Priority > rhs.m_Priority; });

  for (ezUInt32 idx = 0; idx < sorted.GetCount(); ++idx)
  {
    if (!TryPlaceTexture(sorted[idx].m_Index))
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



EZ_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TexturePacker);

