#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Font/FontResource.h>
#include <RendererCore/Font/TextData.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RendererCoreDLL.h>

template <typename Type>
class ezRectTemplate;
typedef ezRectTemplate<ezInt32> ezRectI32;

struct ezTextSpriteAnchor
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    Default = TopLeft
  };
};

struct ezTextHorizontalAlignment
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Left,
    Center,
    Right,

    Default = Left
  };
};

struct ezTextVerticalAlignment
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Top,
    Center,
    Bottom,

    Default = Top
  };
};

struct EZ_RENDERERCORE_DLL ezTextSpriteRenderElementData
{
public:
  ezDynamicArray<ezVec2> m_Vertices;
  ezDynamicArray<ezVec2> m_UVs;
  ezDynamicArray<ezUInt32> m_Indices;
  ezUInt32 m_NumQuads = 0;
  ezTexture2DResourceHandle m_hTexture;
};


struct ezTextSpriteDescriptor
{
  ezString Text;
  ezUInt32 Width;
  ezUInt32 Height;
  ezEnum<ezTextSpriteAnchor> Anchor;
  ezEnum<ezTextHorizontalAlignment> HorizontalAlignment;
  ezEnum<ezTextVerticalAlignment> VerticalAlignment;
  ezFontResourceHandle Font;
  ezUInt32 FontSize;
  ezColor Color;
  bool WrapText;
  bool BreakTextWhenWrapped;
};

class EZ_RENDERERCORE_DLL ezTextSprite
{
public:
  struct Quad
  {
    ezVec2 TopLeft;
    ezVec2 TopRight;
    ezVec2 BottomLeft;
    ezVec2 BottomRight;
  };

public:
  ezTextSprite() = default;
  ~ezTextSprite() = default;
  void Update(const ezTextSpriteDescriptor& desc);

  ezRectI32 GetBounds(const ezVec2I32& offset, const ezRectI32& clipRect) const;

  ezUInt32 FillBuffer(ezDynamicArray<ezVec2>& vertices,
    ezDynamicArray<ezVec2>& uvs,
    ezDynamicArray<ezUInt32>& indices,
    ezUInt32 vertexStride,
    ezUInt32 indexStride,
    ezUInt32 renderElementIndex,
    const ezVec2I32& offset,
    const ezRectI32& clipRect,
    bool clip = true);

  ezUInt32 FillBuffer(ezDynamicArray<ezVec2>& vertices,
    ezDynamicArray<ezVec2>& uvs,
    ezUInt32 vertexStride,
    ezUInt32 renderElementIndex,
    const ezVec2I32& offset,
    const ezRectI32& clipRect,
    bool clip = true);

  static ezVec2I32 GetAnchorOffset(ezEnum<ezTextSpriteAnchor> anchor, ezUInt32 width, ezUInt32 height);

  static void ClipQuadToRect(ezVec2& topLeft,
    ezVec2& topRight,
    ezVec2& bottomLeft,
    ezVec2& bottomRight,
    ezVec2& topLeftUV,
    ezVec2& topRightUV,
    ezVec2& bottomLeftUV,
    ezVec2& bottomRightUV,
    const ezRectI32& clipRect);

  ezUInt32 GetNumRenderElements() const { return m_CachedRenderElements.GetCount(); }

  const ezTextSpriteRenderElementData& GetRenderElementData(ezUInt32 index) const { return m_CachedRenderElements[index]; }

private:
  ezUInt32 GenerateTextureQuads(ezUInt32 page,
    ezTextData& textData,
    ezUInt32 width,
    ezUInt32 height,
    ezEnum<ezTextHorizontalAlignment> hAlignment,
    ezEnum<ezTextVerticalAlignment> vAlignment,
    ezEnum<ezTextSpriteAnchor> anchor,
    ezDynamicArray<ezVec2>& vertices,
    ezDynamicArray<ezVec2>& uvs,
    ezDynamicArray<ezUInt32>& indices,
    ezUInt32 bufferSize);

  void GetAlignmentOffsets(const ezTextData& textData,
    ezUInt32 width,
    ezUInt32 height,
    ezEnum<ezTextHorizontalAlignment> hAlignment,
    ezEnum<ezTextVerticalAlignment> vAlignment,
    ezDynamicArray<ezVec2I32>& output);

  void UpdateBounds();

  void ClearMesh();

private:
  ezRectI32 m_Bounds;
  ezDynamicArray<ezTextSpriteRenderElementData> m_CachedRenderElements;
};
