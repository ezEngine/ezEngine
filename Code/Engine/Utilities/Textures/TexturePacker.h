#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Utilities/Basics.h>

class EZ_UTILITIES_DLL ezTexturePacker
{
public:
  struct Texture
  {
    EZ_DECLARE_POD_TYPE();

    ezVec2U32 m_Size;
    ezVec2U32 m_Position;
    ezInt32 m_Priority = 0;
  };

  ezTexturePacker();
  ~ezTexturePacker();

  void SetTextureSize(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiReserveTextures = 0);

  void AddTexture(ezUInt32 uiWidth, ezUInt32 uiHeight);

  const ezDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  ezResult PackTextures();

private:
  bool CanPlaceAt(ezVec2U32 pos, ezVec2U32 size);
  bool TryPlaceAt(ezVec2U32 pos, ezVec2U32 size);
  ezUInt32 PosToIndex(ezUInt32 x, ezUInt32 y) const;
  bool TryPlaceTexture(ezUInt32 idx);

  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;

  ezDynamicArray<Texture> m_Textures;
  ezDynamicArray<bool> m_Grid;
};
