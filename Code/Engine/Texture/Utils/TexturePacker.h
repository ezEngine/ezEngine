#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

struct stbrp_node;
struct stbrp_rect;

class EZ_TEXTURE_DLL ezTexturePacker
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexturePacker);

public:
  struct Texture
  {
    EZ_DECLARE_POD_TYPE();

    ezVec2U32 m_Size;
    ezVec2U32 m_Position;
  };

  ezTexturePacker();
  ~ezTexturePacker();

  void SetTextureSize(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiReserveTextures = 0);

  void AddTexture(ezUInt32 uiWidth, ezUInt32 uiHeight);

  const ezDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  ezResult PackTextures();

private:
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;

  ezDynamicArray<Texture> m_Textures;
  
  ezDynamicArray<stbrp_node> m_Nodes;
  ezDynamicArray<stbrp_rect> m_Rects;
};
