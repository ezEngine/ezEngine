#include <RendererCorePCH.h>

#include "..\TextSprite.h"
#include <RendererCore/Font/TextSprite.h>

void ezTextSprite::Update(const ezTextSpriteDescriptor& desc)
{
  const ezString32 text(desc.Text);

  ezTextData textData(text, desc.Font, desc.FontSize, desc.Width, desc.Height, desc.WrapText, desc.BreakTextWhenWrapped);

  ezUInt32 numPages = textData.GetNumPages();

  m_CachedRenderElements.Clear();

  m_CachedRenderElements.SetCount(numPages);

  // Generate mesh data
  ezUInt32 texturePage = 0;
  for (auto& cachedRenderElement : m_CachedRenderElements)
  {
    ezUInt32 numQuads = textData.GetNumQuadsForPage(texturePage);

    cachedRenderElement.m_Vertices.SetCountUninitialized(numQuads * 4);
    cachedRenderElement.m_UVs.SetCountUninitialized(numQuads * 4);
    cachedRenderElement.m_Indices.SetCountUninitialized(numQuads * 6);
    cachedRenderElement.m_NumQuads = numQuads;
    cachedRenderElement.m_hTexture = textData.GetTextureForPage(texturePage);

    texturePage++;
  }

  for (ezUInt32 page = 0; page < numPages; page++)
  {
    ezTextSpriteRenderElementData& renderElementData = m_CachedRenderElements[page];

    GenerateTextureQuads(page, textData, desc.Width, desc.Height, desc.HorizontalAlignment, desc.VerticalAlignment, desc.Anchor, renderElementData.m_Vertices, renderElementData.m_UVs, renderElementData.m_Indices, renderElementData.m_NumQuads);
  }
  UpdateBounds();
}

ezRectI32 ezTextSprite::GetBounds(const ezVec2I32& offset, const ezRectI32& clipRect) const
{
  ezRectI32 bounds = m_Bounds;

  if (clipRect.width > 0 && clipRect.height > 0)
  {
    bounds.Clip(clipRect);
  }

  bounds.x += offset.x;
  bounds.y += offset.y;

  return bounds;
}

ezUInt32 ezTextSprite::FillBuffer(ezDynamicArray<ezVec2>& vertices,
  ezDynamicArray<ezVec2>& uvs,
  ezDynamicArray<ezUInt32>& indices,
  ezUInt32 vertexStride,
  ezUInt32 indexStride,
  ezUInt32 renderElementIndex,
  const ezVec2I32& offset,
  const ezRectI32& clipRect,
  bool clip)
{
  ezUInt32 numQuads = FillBuffer(vertices, uvs, vertexStride, renderElementIndex, offset, clipRect, clip);

  const auto& renderElem = m_CachedRenderElements[renderElementIndex];

  indices.GetArrayPtr().CopyFrom(renderElem.m_Indices);

  return numQuads;
}

ezUInt32 ezTextSprite::FillBuffer(ezDynamicArray<ezVec2>& vertices, ezDynamicArray<ezVec2>& uvs, ezUInt32 vertexStride, ezUInt32 renderElementIndex, const ezVec2I32& offset, const ezRectI32& clipRect, bool clip)
{
  const auto& renderElem = m_CachedRenderElements[renderElementIndex];

  ezVec2 vecOffset((float)offset.x, (float)offset.y);

  if (clip)
  {
    for (ezUInt32 i = 0; i < renderElem.m_NumQuads; i++)
    {
      ezUInt32 vertexIndex = i * vertexStride;

      ezVec2& topLeft = vertices[vertexIndex + 0];
      ezVec2& topLeftUV = uvs[vertexIndex + 0];
      topLeft = renderElem.m_Vertices[vertexIndex + 0];
      topLeftUV = renderElem.m_UVs[vertexIndex + 0];

      ezVec2& topRight = vertices[vertexIndex + 1];
      ezVec2& topRightUV = uvs[vertexIndex + 1];
      topRight = renderElem.m_Vertices[vertexIndex + 1];
      topRightUV = renderElem.m_UVs[vertexIndex + 1];

      ezVec2& bottomLeft = vertices[vertexIndex + 2];
      ezVec2& bottomLeftUV = uvs[vertexIndex + 2];
      bottomLeft = renderElem.m_Vertices[vertexIndex + 2];
      bottomLeftUV = renderElem.m_UVs[vertexIndex + 2];

      ezVec2& bottomRight = vertices[vertexIndex + 3];
      ezVec2& bottomRightUV = uvs[vertexIndex + 3];
      bottomRight = renderElem.m_Vertices[vertexIndex + 3];
      bottomRightUV = renderElem.m_UVs[vertexIndex + 3];

      ClipQuadToRect(topLeft, topRight, bottomLeft, bottomRight, topLeftUV, topRightUV, bottomLeftUV, bottomRightUV, clipRect);

      topLeft += vecOffset;
      topRight += vecOffset;
      bottomLeft += vecOffset;
      bottomRight += vecOffset;
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < renderElem.m_NumQuads; i++)
    {
      ezUInt32 vertexIndex = i * vertexStride;

      ezVec2& topLeft = vertices[vertexIndex + 0];
      ezVec2& topLeftUV = uvs[vertexIndex + 0];
      topLeft = renderElem.m_Vertices[vertexIndex + 0];
      topLeftUV = renderElem.m_UVs[vertexIndex + 0];

      ezVec2& topRight = vertices[vertexIndex + 1];
      ezVec2& topRightUV = uvs[vertexIndex + 1];
      topRight = renderElem.m_Vertices[vertexIndex + 1];
      topRightUV = renderElem.m_UVs[vertexIndex + 1];

      ezVec2& bottomLeft = vertices[vertexIndex + 2];
      ezVec2& bottomLeftUV = uvs[vertexIndex + 2];
      bottomLeft = renderElem.m_Vertices[vertexIndex + 2];
      bottomLeftUV = renderElem.m_UVs[vertexIndex + 2];

      ezVec2& bottomRight = vertices[vertexIndex + 3];
      ezVec2& bottomRightUV = uvs[vertexIndex + 3];
      bottomRight = renderElem.m_Vertices[vertexIndex + 3];
      bottomRightUV = renderElem.m_UVs[vertexIndex + 3];

      
  //// flip quad
  //    ezMath::Swap(topLeftUV.x, bottomLeftUV.x);
  //    ezMath::Swap(topLeftUV.y, bottomLeftUV.y);

  //    ezMath::Swap(topRightUV.x, bottomRightUV.x);
  //    ezMath::Swap(topRightUV.y, bottomRightUV.y);

      topLeft += vecOffset;
      topRight += vecOffset;
      bottomLeft += vecOffset;
      bottomRight += vecOffset;
    }
  }

  return renderElem.m_NumQuads;
}

// Vertices in the quad must be in a specific order: top left, top right, bottom left, bottom right
// (0, 0) represents top left of the screen
void ezTextSprite::ClipQuadToRect(ezVec2& topLeft, ezVec2& topRight, ezVec2& bottomLeft, ezVec2& bottomRight, ezVec2& topLeftUV, ezVec2& topRightUV, ezVec2& bottomLeftUV, ezVec2& bottomRightUV, const ezRectI32& clipRect)
{
  float left = (float)clipRect.Left();
  float right = (float)clipRect.Right();
  float top = (float)clipRect.Top();
  float bottom = (float)clipRect.Bottom();

  // Attempt to skip those that are definitely not clipped
  if (topLeft.x >= left && topRight.x <= right && topLeft.y >= top && bottomLeft.y <= bottom)
    return;

  // TODO - Skip those that are 100% clipped as well

  float du = (topRightUV.x - topLeftUV.x) / (topRight.x - topLeft.x);
  float dv = (topLeftUV.y - bottomLeftUV.y) / (topLeft.y - bottomRight.y);

  if (right < left)
    ezMath::Swap(left, right);

  if (bottom < top)
    ezMath::Swap(bottom, top);

  // Clip left
  float newLeft = ezMath::Clamp(topLeft.x, left, right);
  float uvLeftOffset = (newLeft - topLeft.x) * du;

  // Clip right
  float newRight = ezMath::Clamp(topRight.x, left, right);
  float uvRightOffset = (topRight.x - newRight) * du;

  // Clip top
  float newTop = ezMath::Clamp(topLeft.y, top, bottom);
  float uvTopOffset = (newTop - topLeft.y) * dv;

  // Clip bottom
  float newBottom = ezMath::Clamp(bottomLeft.y, top, bottom);
  float uvBottomOffset = (bottomLeft.y - newBottom) * dv;

  topLeft.x = newLeft;
  bottomLeft.x = newLeft;
  topRight.x = newRight;
  bottomRight.x = newRight;
  topLeft.y = newTop;
  topRight.y = newTop;
  bottomLeft.y = newBottom;
  bottomRight.y = newBottom;

  topLeftUV.x += uvLeftOffset;
  bottomLeftUV.x += uvLeftOffset;
  topRightUV.x -= uvRightOffset;
  bottomLeftUV.x -= uvRightOffset;
  topLeftUV.y += uvTopOffset;
  topRightUV.y += uvTopOffset;
  bottomLeftUV.y -= uvBottomOffset;
  bottomLeftUV.y -= uvBottomOffset;
}

void ezTextSprite::GetAlignmentOffsets(const ezTextData& textData, ezUInt32 width, ezUInt32 height, ezEnum<ezTextHorizontalAlignment> hAlignment, ezEnum<ezTextVerticalAlignment> vAlignment, ezDynamicArray<ezVec2I32>& output)
{
  ezUInt32 numLines = textData.GetNumLines();
  ezUInt32 currentHeight = 0;

  for (ezUInt32 i = 0; i < numLines; i++)
  {
    const ezTextData::ezTextLine& line = textData.GetLine(i);
    currentHeight += line.GetYOffset();
  }

  // Calculate vertical alignment offset
  const ezUInt32 vDifference = (ezUInt32)ezMath::Max<ezInt32>(0, (ezInt32)height - (ezInt32)currentHeight);
  ezUInt32 vOffset = 0;
  switch (vAlignment)
  {
    case ezTextVerticalAlignment::Top:
      vOffset = 0;
      break;

    case ezTextVerticalAlignment::Center:
      vOffset = (ezUInt32)ezMath::Max<ezInt32>(0, (ezInt32)vDifference) / 2;
      break;

    case ezTextVerticalAlignment::Bottom:
      vOffset = (ezUInt32)ezMath::Max<ezInt32>(0, (ezInt32)vDifference);
      break;
  }

  // Calculate horizontal alignment offset
  ezUInt32 currentY = 0;
  for (ezUInt32 i = 0; i < numLines; i++)
  {
    const ezTextData::ezTextLine& line = textData.GetLine(i);

    ezUInt32 hOffset = 0;
    switch (hAlignment)
    {
      case ezTextHorizontalAlignment::Left:
        hOffset = 0;
        break;

      case ezTextHorizontalAlignment::Center:
        hOffset = (ezUInt32)ezMath::Max<ezInt32>(0, (ezInt32)(width - line.GetWidth())) / 2;
        break;

      case ezTextHorizontalAlignment::Right:
        hOffset = (ezUInt32)ezMath::Max<ezInt32>(0, (ezInt32)(width - line.GetWidth()));
        break;
    }

    output[i] = ezVec2I32(hOffset, vOffset + currentY);
    currentY += line.GetYOffset();
  }
}

ezVec2I32 ezTextSprite::GetAnchorOffset(ezEnum<ezTextSpriteAnchor> anchor, ezUInt32 width, ezUInt32 height)
{
  switch (anchor)
  {
    case ezTextSpriteAnchor::TopLeft:
      return ezVec2I32(0, 0);

    case ezTextSpriteAnchor::TopCenter:
      return ezVec2I32(width / 2, 0);

    case ezTextSpriteAnchor::TopRight:
      return ezVec2I32(width, 0);

    case ezTextSpriteAnchor::MiddleLeft:
      return ezVec2I32(0, height / 2);

    case ezTextSpriteAnchor::MiddleCenter:
      return ezVec2I32(width / 2, height / 2);

    case ezTextSpriteAnchor::MiddleRight:
      return ezVec2I32(width, height / 2);

    case ezTextSpriteAnchor::BottomLeft:
      return ezVec2I32(0, height);

    case ezTextSpriteAnchor::BottomCenter:
      return ezVec2I32(width / 2, height);

    case ezTextSpriteAnchor::BottomRight:
      return ezVec2I32(width, height);
  }

  return ezVec2I32(0, 0);
}

void ezTextSprite::UpdateBounds()
{
  ezVec2 min;
  ezVec2 max;

  // Find starting point
  bool startingPointFound = false;

  for (auto& renderElementData : m_CachedRenderElements)
  {
    if (renderElementData.m_Vertices.GetCount() > 0 && renderElementData.m_NumQuads > 0)
    {
      min = renderElementData.m_Vertices[0];
      max = renderElementData.m_Vertices[0];

      startingPointFound = true;
      break;
    }
  }

  if (!startingPointFound)
  {
    m_Bounds = ezRectI32(0, 0, 0, 0);
    return;
  }

  // Calculate bounds
  for (auto& renderElementData : m_CachedRenderElements)
  {
    if (renderElementData.m_Vertices.GetCount() > 0 && renderElementData.m_NumQuads > 0)
    {
      ezUInt32 vertexCount = renderElementData.m_NumQuads * 4;

      for (ezUInt32 i = 0; i < vertexCount; i++)
      {
        min = min.CompMin(renderElementData.m_Vertices[i]);
        max = min.CompMax(renderElementData.m_Vertices[i]);
      }
    }
  }
  m_Bounds = ezRectI32(min.x, min.y, max.x - min.x, max.y - min.y);
}

void ezTextSprite::ClearMesh()
{
  m_CachedRenderElements.Clear();

  UpdateBounds();
}

ezUInt32 ezTextSprite::GenerateTextureQuads(ezUInt32 page,
  ezTextData& textData,
  ezUInt32 width,
  ezUInt32 height,
  ezEnum<ezTextHorizontalAlignment> hAlignment,
  ezEnum<ezTextVerticalAlignment> vAlignment,
  ezEnum<ezTextSpriteAnchor> anchor,
  ezDynamicArray<ezVec2>& vertices,
  ezDynamicArray<ezVec2>& uvs,
  ezDynamicArray<ezUInt32>& indices,
  ezUInt32 bufferSize)
{
  ezUInt32 numLines = textData.GetNumLines();
  ezUInt32 numQuads = textData.GetNumQuadsForPage(page);

  ezDynamicArray<ezVec2I32> alignmentOffsets;
  alignmentOffsets.SetCountUninitialized(numLines);

  GetAlignmentOffsets(textData, width, height, hAlignment, vAlignment, alignmentOffsets);
  ezVec2I32 offset = GetAnchorOffset(anchor, width, height);

  ezUInt32 quadOffset = 0;

  for (ezUInt32 lineNum = 0; lineNum < numLines; lineNum++)
  {
    const ezTextData::ezTextLine& line = textData.GetLine(lineNum);
    ezUInt32 writtenQuads = line.FillBuffer(page, vertices, uvs, indices, quadOffset, bufferSize);

    ezVec2I32 position = offset + alignmentOffsets[lineNum];
    ezUInt32 numVertices = writtenQuads * 4;

    for (ezUInt32 i = 0; i < numVertices; i++)
    {
      ezVec2& vertex = vertices[quadOffset * 4 + i];
      vertex.x += (float)position.x;
      vertex.y += (float)position.y;
    }

    quadOffset += writtenQuads;
  }

  alignmentOffsets.Clear();

  return numQuads;
}
