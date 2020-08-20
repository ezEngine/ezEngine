#include <RendererCore/Font/TextSprite.h>
#include <RendererCorePCH.h>

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
    //bounds.Clip(clipRect);
  }

  bounds.x += offset.x;
  bounds.y += offset.y;

  return bounds;
}

ezUInt32 ezTextSprite::FillBuffer(ezDynamicArray<ezUInt8> vertices,
  ezDynamicArray<ezUInt32> uvs,
  ezDynamicArray<ezUInt32> indices,
  ezUInt32 indexOffset,
  ezUInt32 maxNumVerts,
  ezUInt32 maxNumIndices,
  ezUInt32 vertexStride,
  ezUInt32 indexStride,
  ezUInt32 renderElementIndex,
  const ezVec2I32 offset,
  ezRectI32 clipRect,
  bool clip)
{
  return ezUInt32();
}

// This will only properly clip an array of quads
// Vertices in the quad must be in a specific order: top left, top right, bottom left, bottom right
// (0, 0) represents top left of the screen
void ezTextSprite::ClipQuadsToRect(ezDynamicArray<ezUInt8>& inoutVertices, ezDynamicArray<ezUInt8>& inoutUVs, ezUInt32 numQuads, ezUInt32 vertexStride, const ezRectI32& clipRect)
{
  float left = clipRect.Left();
  float right = clipRect.Right();
  float top = clipRect.Top();
  float bottom = clipRect.Bottom();

  for (ezUInt32 i = 0; i < numQuads; i++)
  {

  }
}

void ezTextSprite::ClipTrianglesToRect(ezDynamicArray<ezUInt8>& inoutVertices, ezDynamicArray<ezUInt8>& inoutUVs, ezUInt32 numTriangles, ezUInt32 vertexStride, const ezRectI32& clipRect, const ezDelegate<void(ezDynamicArray<ezVec2>&, ezDynamicArray<ezVec2>&, ezUInt32)>& writeCallback)
{
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
      ezVec2 vertex = vertices.GetData()[quadOffset * 4 + i];
      vertex.x += (float)position.x;
      vertex.y += (float)position.y;
    }

    quadOffset += writtenQuads;
  }

  alignmentOffsets.Clear();

  return numQuads;
}
