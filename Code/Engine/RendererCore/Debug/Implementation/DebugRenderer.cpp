#include <RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/SimpleASCIIFont.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

//////////////////////////////////////////////////////////////////////////

ezDebugRendererContext::ezDebugRendererContext(const ezWorld* pWorld)
    : m_Id(pWorld != nullptr ? pWorld->GetIndex() : 0)
{
}

ezDebugRendererContext::ezDebugRendererContext(const ezViewHandle& hView)
    : m_Id(hView.GetInternalID().m_Data)
{
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  struct EZ_ALIGN_16(Vertex)
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(Vertex) == 16);

  struct EZ_ALIGN_16(TexVertex)
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
    ezVec2 m_texCoord;
    float padding[2];
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(TexVertex) == 32);

  struct EZ_ALIGN_16(BoxData)
  {
    ezShaderTransform m_transform;
    ezColor m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(BoxData) == 64);

  struct EZ_ALIGN_16(GlyphData)
  {
    ezVec2 m_topLeftCorner;
    ezColorLinearUB m_color;
    ezUInt16 m_glyphIndex;
    ezUInt16 m_sizeInPixel;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(GlyphData) == 16);

  struct TextLineData2D
  {
    ezString m_text;
    ezVec2 m_topLeftCorner;
    ezColorLinearUB m_color;
    ezUInt32 m_uiSizeInPixel;
  };

  struct TextLineData3D : public TextLineData2D
  {
    ezVec3 m_position;
  };

  struct PerContextData
  {
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_lineVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangleVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangle2DVertices;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_lineBoxes;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_solidBoxes;
    ezMap<ezTexture2DResourceHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle2DVertices;

    ezDynamicArray<TextLineData2D> m_textLines2D;
    ezDynamicArray<TextLineData3D> m_textLines3D;
    ezDynamicArray<GlyphData, ezAlignedAllocatorWrapper> m_glyphs;
  };

  struct DoubleBufferedPerContextData
  {
    DoubleBufferedPerContextData()
    {
      m_uiLastRenderedFrame = 0;
      m_pData[0] = nullptr;
      m_pData[1] = nullptr;
    }

    ezUInt64 m_uiLastRenderedFrame;
    PerContextData* m_pData[2];
  };

  static ezHashTable<ezDebugRendererContext, DoubleBufferedPerContextData> s_PerContextData;
  static ezMutex s_Mutex;

  static PerContextData& GetDataForExtraction(const ezDebugRendererContext& context)
  {
    DoubleBufferedPerContextData& doubleBufferedData = s_PerContextData[context];

    const ezUInt32 uiDataIndex =
        ezRenderWorld::IsRenderingThread() && (doubleBufferedData.m_uiLastRenderedFrame != ezRenderWorld::GetFrameCounter())
            ? ezRenderWorld::GetDataIndexForRendering()
            : ezRenderWorld::GetDataIndexForExtraction();

    PerContextData* pData = doubleBufferedData.m_pData[uiDataIndex];
    if (pData == nullptr)
    {
      pData = EZ_DEFAULT_NEW(PerContextData);
      doubleBufferedData.m_pData[uiDataIndex] = pData;
    }

    return *pData;
  }

  static void ClearRenderData(ezUInt64)
  {
    EZ_LOCK(s_Mutex);

    for (auto it = s_PerContextData.GetIterator(); it.IsValid(); ++it)
    {
      PerContextData* pData = it.Value().m_pData[ezRenderWorld::GetDataIndexForRendering()];
      if (pData)
      {
        pData->m_lineVertices.Clear();
        pData->m_lineBoxes.Clear();
        pData->m_solidBoxes.Clear();
        pData->m_triangleVertices.Clear();
        pData->m_triangle2DVertices.Clear();
        pData->m_texTriangle2DVertices.Clear();
        pData->m_textLines2D.Clear();
        pData->m_textLines3D.Clear();
      }
    }
  }

  struct BufferType
  {
    enum Enum
    {
      Lines,
      LineBoxes,
      SolidBoxes,
      Triangles3D,
      Triangles2D,
      TexTriangles2D,
      Glyphs,

      Count
    };
  };

  static ezGALBufferHandle s_hDataBuffer[BufferType::Count];

  static ezMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  static ezMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  static ezVertexDeclarationInfo s_VertexDeclarationInfo;
  static ezVertexDeclarationInfo s_TexVertexDeclarationInfo;
  static ezTexture2DResourceHandle s_hDebugFontTexture;

  static ezShaderResourceHandle s_hDebugGeometryShader;
  static ezShaderResourceHandle s_hDebugPrimitiveShader;
  static ezShaderResourceHandle s_hDebugTexturedPrimitiveShader;
  static ezShaderResourceHandle s_hDebugTextShader;

  enum
  {
    DEBUG_BUFFER_SIZE = 1024 * 256,
    BOXES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
    TEX_TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(TexVertex) / 3) * 3,
    GLYPHS_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(GlyphData),
  };

  static void CreateDataBuffer(BufferType::Enum bufferType, ezUInt32 uiStructSize)
  {
    if (s_hDataBuffer[bufferType].IsInvalidated())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiStructSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferType = ezGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_hDataBuffer[bufferType] = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  static void CreateVertexBuffer(BufferType::Enum bufferType, ezUInt32 uiVertexSize)
  {
    if (s_hDataBuffer[bufferType].IsInvalidated())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiVertexSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferType = ezGALBufferType::VertexBuffer;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_hDataBuffer[bufferType] = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  static void DestroyBuffer(BufferType::Enum bufferType)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    if (!s_hDataBuffer[bufferType].IsInvalidated())
    {
      pDevice->DestroyBuffer(s_hDataBuffer[bufferType]);

      s_hDataBuffer[bufferType].Invalidate();
    }
  }

  template <typename AddFunc>
  static void AddTextLines(const ezDebugRendererContext& context, ezStringView text, const ezVec2I32& positionInPixel, float fSizeInPixel,
                           ezDebugRenderer::HorizontalAlignment::Enum horizontalAlignment,
                           ezDebugRenderer::VerticalAlignment::Enum verticalAlignment, AddFunc func)
  {
    if (text.IsEmpty())
      return;

    ezHybridArray<ezStringView, 8> lines;
    ezUInt32 maxLineLength = 0;

    if (text.FindSubString("\\n"))
    {
      ezStringBuilder sb = text;
      sb.Split(false, lines, "\\n");

      for (auto& line : lines)
      {
        maxLineLength = ezMath::Max(maxLineLength, line.GetElementCount());
      }
    }
    else
    {
      lines.PushBack(text);
      maxLineLength = text.GetElementCount();
    }

    // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
    const float fGlyphWidth = ezMath::Ceil(fSizeInPixel * (8.0f / 16.0f));
    const float fLineHeight = ezMath::Ceil(fSizeInPixel * (20.0f / 16.0f));

    float screenPosX = (float)positionInPixel.x;
    if (horizontalAlignment == ezDebugRenderer::HorizontalAlignment::Right)
      screenPosX -= maxLineLength * fGlyphWidth;

    float screenPosY = (float)positionInPixel.y;
    if (verticalAlignment == ezDebugRenderer::VerticalAlignment::Center)
      screenPosY -= ezMath::Ceil(lines.GetCount() * fLineHeight * 0.5f);
    else if (verticalAlignment == ezDebugRenderer::VerticalAlignment::Bottom)
      screenPosY -= lines.GetCount() * fLineHeight;

    {
      EZ_LOCK(s_Mutex);

      auto& data = GetDataForExtraction(context);

      ezVec2 currentPos(screenPosX, screenPosY);

      for (ezStringView line : lines)
      {
        currentPos.x = screenPosX;
        if (horizontalAlignment == ezDebugRenderer::HorizontalAlignment::Center)
          currentPos.x -= ezMath::Ceil(line.GetElementCount() * fGlyphWidth * 0.5f);

        func(data, line, currentPos);

        currentPos.y += fLineHeight;
      }
    }
  }

  static void AppendGlyphs(ezDynamicArray<GlyphData, ezAlignedAllocatorWrapper>& glyphs, const TextLineData2D& textLine)
  {
    ezVec2 currentPos = textLine.m_topLeftCorner;
    const float fGlyphWidth = ezMath::Ceil(textLine.m_uiSizeInPixel * (8.0f / 16.0f));

    for (ezUInt32 uiCharacter : textLine.m_text)
    {
      auto& glyphData = glyphs.ExpandAndGetRef();
      glyphData.m_topLeftCorner = currentPos;
      glyphData.m_color = textLine.m_color;
      glyphData.m_glyphIndex = uiCharacter < 128 ? uiCharacter : 0;
      glyphData.m_sizeInPixel = (ezUInt16)textLine.m_uiSizeInPixel;

      currentPos.x += fGlyphWidth;
    }
  }
} // namespace

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DebugRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezDebugRenderer::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezDebugRenderer::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
void ezDebugRenderer::DrawLines(const ezDebugRendererContext& context, ezArrayPtr<Line> lines, const ezColor& color)
{
  if (lines.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    ezVec3* pPositions = &line.m_start;

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_lineVertices.ExpandAndGetRef();
      vertex.m_position = pPositions[i];
      vertex.m_color = color;
    }
  }
}

// static
void ezDebugRenderer::DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color,
                                  const ezTransform& transform)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();

  ezTransform boxTransform(box.GetCenter(), ezQuat::IdentityQuaternion(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform;
  boxData.m_color = color;
}

// static
void ezDebugRenderer::DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction,
                                         const ezColor& color, const ezTransform& transform)
{
  fCornerFraction = ezMath::Clamp(fCornerFraction, 0.0f, 1.0f) * 0.5f;

  ezVec3 corners[8];
  box.GetCorners(corners);

  for (ezUInt32 i = 0; i < 8; ++i)
  {
    corners[i] = transform * corners[i];
  }

  ezVec3 edgeEnds[12];
  edgeEnds[0] = corners[1];  // 0 -> 1
  edgeEnds[1] = corners[3];  // 1 -> 3
  edgeEnds[2] = corners[0];  // 2 -> 0
  edgeEnds[3] = corners[2];  // 3 -> 2
  edgeEnds[4] = corners[5];  // 4 -> 5
  edgeEnds[5] = corners[7];  // 5 -> 7
  edgeEnds[6] = corners[4];  // 6 -> 4
  edgeEnds[7] = corners[6];  // 7 -> 6
  edgeEnds[8] = corners[4];  // 0 -> 4
  edgeEnds[9] = corners[5];  // 1 -> 5
  edgeEnds[10] = corners[6]; // 2 -> 6
  edgeEnds[11] = corners[7]; // 3 -> 7

  Line lines[24];
  for (ezUInt32 i = 0; i < 12; ++i)
  {
    ezVec3 edgeStart = corners[i % 8];
    ezVec3 edgeEnd = edgeEnds[i];
    ezVec3 edgeDir = edgeEnd - edgeStart;

    lines[i * 2 + 0].m_start = edgeStart;
    lines[i * 2 + 0].m_end = edgeStart + edgeDir * fCornerFraction;

    lines[i * 2 + 1].m_start = edgeEnd;
    lines[i * 2 + 1].m_end = edgeEnd - edgeDir * fCornerFraction;
  }

  DrawLines(context, lines, color);
}

// static
void ezDebugRenderer::DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color,
                                     const ezTransform& transform /*= ezTransform::IdentityTransform()*/)
{
  enum
  {
    NUM_SEGMENTS = 32
  };

  const ezVec3 vCenter = sphere.m_vCenter;
  const float fRadius = sphere.m_fRadius;
  const ezAngle stepAngle = ezAngle::Degree(360.0f / NUM_SEGMENTS);

  Line lines[NUM_SEGMENTS * 3];
  for (ezUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = ezMath::Cos(fS1 * stepAngle);
    const float fCos2 = ezMath::Cos(fS2 * stepAngle);

    const float fSin1 = ezMath::Sin(fS1 * stepAngle);
    const float fSin2 = ezMath::Sin(fS2 * stepAngle);

    lines[s * 3 + 0].m_start = transform * (vCenter + ezVec3(0.0f, fCos1, fSin1) * fRadius);
    lines[s * 3 + 0].m_end = transform * (vCenter + ezVec3(0.0f, fCos2, fSin2) * fRadius);

    lines[s * 3 + 1].m_start = transform * (vCenter + ezVec3(fCos1, 0.0f, fSin1) * fRadius);
    lines[s * 3 + 1].m_end = transform * (vCenter + ezVec3(fCos2, 0.0f, fSin2) * fRadius);

    lines[s * 3 + 2].m_start = transform * (vCenter + ezVec3(fCos1, fSin1, 0.0f) * fRadius);
    lines[s * 3 + 2].m_end = transform * (vCenter + ezVec3(fCos2, fSin2, 0.0f) * fRadius);
  }

  DrawLines(context, lines, color);
}


void ezDebugRenderer::DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color,
                                       const ezTransform& transform /*= ezTransform::IdentityTransform()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const ezAngle stepAngle = ezAngle::Degree(360.0f / NUM_SEGMENTS);

  Line lines[NUM_LINES];

  const float fOffsetZ = fLength * 0.5f;

  ezUInt32 curLine = 0;

  // render 4 straight lines
  lines[curLine].m_start = transform * ezVec3(-fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * ezVec3(-fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * ezVec3(+fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * ezVec3(+fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * ezVec3(0, -fRadius, fOffsetZ);
  lines[curLine].m_end = transform * ezVec3(0, -fRadius, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * ezVec3(0, +fRadius, fOffsetZ);
  lines[curLine].m_end = transform * ezVec3(0, +fRadius, -fOffsetZ);
  ++curLine;

  // render top and bottom circle
  for (ezUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = ezMath::Cos(fS1 * stepAngle);
    const float fCos2 = ezMath::Cos(fS2 * stepAngle);

    const float fSin1 = ezMath::Sin(fS1 * stepAngle);
    const float fSin2 = ezMath::Sin(fS2 * stepAngle);

    lines[curLine].m_start = transform * ezVec3(fCos1 * fRadius, fSin1 * fRadius, fOffsetZ);
    lines[curLine].m_end = transform * ezVec3(fCos2 * fRadius, fSin2 * fRadius, fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * ezVec3(fCos1 * fRadius, fSin1 * fRadius, -fOffsetZ);
    lines[curLine].m_end = transform * ezVec3(fCos2 * fRadius, fSin2 * fRadius, -fOffsetZ);
    ++curLine;
  }

  // render top and bottom half circles
  for (ezUInt32 s = 0; s < NUM_HALF_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = ezMath::Cos(fS1 * stepAngle);
    const float fCos2 = ezMath::Cos(fS2 * stepAngle);

    const float fSin1 = ezMath::Sin(fS1 * stepAngle);
    const float fSin2 = ezMath::Sin(fS2 * stepAngle);

    // top two bows
    lines[curLine].m_start = transform * ezVec3(0.0f, fCos1 * fRadius, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_end = transform * ezVec3(0.0f, fCos2 * fRadius, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * ezVec3(fCos1 * fRadius, 0.0f, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_end = transform * ezVec3(fCos2 * fRadius, 0.0f, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    // bottom two bows
    lines[curLine].m_start = transform * ezVec3(0.0f, fCos1 * fRadius, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_end = transform * ezVec3(0.0f, fCos2 * fRadius, -fSin2 * fRadius - fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * ezVec3(fCos1 * fRadius, 0.0f, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_end = transform * ezVec3(fCos2 * fRadius, 0.0f, -fSin2 * fRadius - fOffsetZ);
    ++curLine;
  }

  EZ_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

// static
void ezDebugRenderer::DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color,
                                      bool bDrawPlaneNormals /*= false*/)
{
  ezVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints);

  Line lines[12] = {
      Line(cornerPoints[0], cornerPoints[1]), Line(cornerPoints[1], cornerPoints[2]),
      Line(cornerPoints[2], cornerPoints[3]), Line(cornerPoints[3], cornerPoints[0]),

      Line(cornerPoints[4], cornerPoints[5]), Line(cornerPoints[5], cornerPoints[6]),
      Line(cornerPoints[6], cornerPoints[7]), Line(cornerPoints[7], cornerPoints[4]),

      Line(cornerPoints[0], cornerPoints[4]), Line(cornerPoints[1], cornerPoints[5]),
      Line(cornerPoints[2], cornerPoints[6]), Line(cornerPoints[3], cornerPoints[7]),
  };

  DrawLines(context, lines, color);

  if (bDrawPlaneNormals)
  {
    ezColor normalColor = color + ezColor(0.4f, 0.4f, 0.4f);
    float fDrawLength = 0.5f;

    ezVec3 nearPlaneNormal = frustum.GetPlane(0).m_vNormal * fDrawLength;
    ezVec3 farPlaneNormal = frustum.GetPlane(1).m_vNormal * fDrawLength;
    ezVec3 leftPlaneNormal = frustum.GetPlane(2).m_vNormal * fDrawLength;
    ezVec3 rightPlaneNormal = frustum.GetPlane(3).m_vNormal * fDrawLength;
    ezVec3 bottomPlaneNormal = frustum.GetPlane(4).m_vNormal * fDrawLength;
    ezVec3 topPlaneNormal = frustum.GetPlane(5).m_vNormal * fDrawLength;

    Line normalLines[24] = {
        Line(cornerPoints[0], cornerPoints[0] + nearPlaneNormal),   Line(cornerPoints[1], cornerPoints[1] + nearPlaneNormal),
        Line(cornerPoints[2], cornerPoints[2] + nearPlaneNormal),   Line(cornerPoints[3], cornerPoints[3] + nearPlaneNormal),

        Line(cornerPoints[4], cornerPoints[4] + farPlaneNormal),    Line(cornerPoints[5], cornerPoints[5] + farPlaneNormal),
        Line(cornerPoints[6], cornerPoints[6] + farPlaneNormal),    Line(cornerPoints[7], cornerPoints[7] + farPlaneNormal),

        Line(cornerPoints[0], cornerPoints[0] + leftPlaneNormal),   Line(cornerPoints[3], cornerPoints[3] + leftPlaneNormal),
        Line(cornerPoints[4], cornerPoints[4] + leftPlaneNormal),   Line(cornerPoints[7], cornerPoints[7] + leftPlaneNormal),

        Line(cornerPoints[1], cornerPoints[1] + rightPlaneNormal),  Line(cornerPoints[2], cornerPoints[2] + rightPlaneNormal),
        Line(cornerPoints[5], cornerPoints[5] + rightPlaneNormal),  Line(cornerPoints[6], cornerPoints[6] + rightPlaneNormal),

        Line(cornerPoints[2], cornerPoints[2] + bottomPlaneNormal), Line(cornerPoints[3], cornerPoints[3] + bottomPlaneNormal),
        Line(cornerPoints[6], cornerPoints[6] + bottomPlaneNormal), Line(cornerPoints[7], cornerPoints[7] + bottomPlaneNormal),

        Line(cornerPoints[0], cornerPoints[0] + topPlaneNormal),    Line(cornerPoints[1], cornerPoints[1] + topPlaneNormal),
        Line(cornerPoints[4], cornerPoints[4] + topPlaneNormal),    Line(cornerPoints[5], cornerPoints[5] + topPlaneNormal),
    };

    DrawLines(context, normalLines, normalColor);
  }
}

// static
void ezDebugRenderer::DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color,
                                   const ezTransform& transform)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_solidBoxes.ExpandAndGetRef();

  ezTransform boxTransform(box.GetCenter(), ezQuat::IdentityQuaternion(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform;
  boxData.m_color = color;
}

// static
void ezDebugRenderer::DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<Triangle> triangles, const ezColor& color)
{
  if (triangles.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& triangle : triangles)
  {
    ezVec3* pPositions = &triangle.m_p0;

    for (ezUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.m_triangleVertices.ExpandAndGetRef();
      vertex.m_position = pPositions[i];
      vertex.m_color = color;
    }
  }
}

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth,
                                      const ezColor& color)
{
  Vertex vertices[6];

  vertices[0].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[1].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[3].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[4].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[5].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_triangle2DVertices.PushBackRange(ezMakeArrayPtr(vertices));
}

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth,
                                      const ezColor& color, const ezTexture2DResourceHandle& hTexture)
{
  TexVertex vertices[6];

  vertices[0].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[0].m_texCoord = ezVec2(0, 0);
  vertices[1].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[1].m_texCoord = ezVec2(1, 1);
  vertices[2].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_texCoord = ezVec2(0, 1);
  vertices[3].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[3].m_texCoord = ezVec2(0, 0);
  vertices[4].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[4].m_texCoord = ezVec2(1, 0);
  vertices[5].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[5].m_texCoord = ezVec2(1, 1);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_texTriangle2DVertices[hTexture].PushBackRange(ezMakeArrayPtr(vertices));
}

void ezDebugRenderer::Draw2DText(const ezDebugRendererContext& context, const ezStringView& text, const ezVec2I32& positionInPixel,
                               const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/,
                               HorizontalAlignment::Enum horizontalAlignment /*= HorizontalAlignment::Left*/,
                               VerticalAlignment::Enum verticalAlignment /*= VerticalAlignment::Top*/)
{
  AddTextLines(context, text, positionInPixel, (float)uiSizeInPixel, horizontalAlignment, verticalAlignment,
    [=](PerContextData& data, ezStringView line, ezVec2 topLeftCorner)
    {
      auto& textLine = data.m_textLines2D.ExpandAndGetRef();
      textLine.m_text = line;
      textLine.m_topLeftCorner = topLeftCorner;
      textLine.m_color = color;
      textLine.m_uiSizeInPixel = uiSizeInPixel;
    });
}


void ezDebugRenderer::Draw3DText(const ezDebugRendererContext& context, const ezStringView& text, const ezVec3& globalPosition,
                                 const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/,
                                 HorizontalAlignment::Enum horizontalAlignment /*= HorizontalAlignment::Left*/,
                                 VerticalAlignment::Enum verticalAlignment /*= VerticalAlignment::Top*/)
{
  AddTextLines(context, text, ezVec2I32(0), (float)uiSizeInPixel, horizontalAlignment, verticalAlignment,
    [=](PerContextData& data, ezStringView line, ezVec2 topLeftCorner)
    {
      auto& textLine = data.m_textLines3D.ExpandAndGetRef();
      textLine.m_text = line;
      textLine.m_topLeftCorner = topLeftCorner;
      textLine.m_color = color;
      textLine.m_uiSizeInPixel = uiSizeInPixel;
      textLine.m_position = globalPosition;
    });
}

// static
void ezDebugRenderer::Render(const ezRenderViewContext& renderViewContext)
{
  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternal(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternal(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

// static
void ezDebugRenderer::RenderInternal(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext)
{
  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
  {
    return;
  }

  pDoubleBufferedContextData->m_uiLastRenderedFrame = ezRenderWorld::GetFrameCounter();

  PerContextData* pData = pDoubleBufferedContextData->m_pData[ezRenderWorld::GetDataIndexForRendering()];
  if (pData == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  // SolidBoxes
  {
    ezUInt32 uiNumSolidBoxes = pData->m_solidBoxes.GetCount();
    if (uiNumSolidBoxes != 0)
    {
      CreateDataBuffer(BufferType::SolidBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindBuffer("boxData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::SolidBoxes]));
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);

      const BoxData* pSolidBoxData = pData->m_solidBoxes.GetData();
      while (uiNumSolidBoxes > 0)
      {
        const ezUInt32 uiNumSolidBoxesInBatch = ezMath::Min<ezUInt32>(uiNumSolidBoxes, BOXES_PER_BATCH);
        pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::SolidBoxes], 0,
                                  ezMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumSolidBoxesInBatch);

        uiNumSolidBoxes -= uiNumSolidBoxesInBatch;
        pSolidBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // Triangles
  {
    ezUInt32 uiNumTriangleVertices = pData->m_triangleVertices.GetCount();
    if (uiNumTriangleVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles3D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_triangleVertices.GetData();
      while (uiNumTriangleVertices > 0)
      {
        const ezUInt32 uiNumTriangleVerticesInBatch = ezMath::Min<ezUInt32>(uiNumTriangleVertices, TRIANGLE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumTriangleVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::Triangles3D], 0,
                                  ezMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles3D], ezGALBufferHandle(),
                                                           &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles,
                                                           uiNumTriangleVerticesInBatch / 3);
        renderViewContext.m_pRenderContext->DrawMeshBuffer();

        uiNumTriangleVertices -= uiNumTriangleVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Lines
  {
    ezUInt32 uiNumLineVertices = pData->m_lineVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_lineVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::Lines], 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Lines], ezGALBufferHandle(), &s_VertexDeclarationInfo,
                                                           ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);
        renderViewContext.m_pRenderContext->DrawMeshBuffer();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // LineBoxes
  {
    ezUInt32 uiNumLineBoxes = pData->m_lineBoxes.GetCount();
    if (uiNumLineBoxes != 0)
    {
      CreateDataBuffer(BufferType::LineBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindBuffer("boxData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::LineBoxes]));
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);

      const BoxData* pLineBoxData = pData->m_lineBoxes.GetData();
      while (uiNumLineBoxes > 0)
      {
        const ezUInt32 uiNumLineBoxesInBatch = ezMath::Min<ezUInt32>(uiNumLineBoxes, BOXES_PER_BATCH);
        pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::LineBoxes], 0,
                                  ezMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch);

        uiNumLineBoxes -= uiNumLineBoxesInBatch;
        pLineBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // 2D Rectangles
  {
    ezUInt32 uiNum2DVertices = pData->m_triangle2DVertices.GetCount();
    if (uiNum2DVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles2D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_triangle2DVertices.GetData();
      while (uiNum2DVertices > 0)
      {
        const ezUInt32 uiNum2DVerticesInBatch = ezMath::Min<ezUInt32>(uiNum2DVertices, TRIANGLE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::Triangles2D], 0,
                                  ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles2D], ezGALBufferHandle(),
                                                           &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles,
                                                           uiNum2DVerticesInBatch / 3);
        renderViewContext.m_pRenderContext->DrawMeshBuffer();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 2D Rectangles
  {
    for (auto itTex = pData->m_texTriangle2DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", itTex.Key());

      const auto& verts = itTex.Value();

      ezUInt32 uiNum2DVertices = verts.GetCount();
      if (uiNum2DVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles2D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNum2DVertices > 0)
        {
          const ezUInt32 uiNum2DVerticesInBatch = ezMath::Min<ezUInt32>(uiNum2DVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          EZ_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::TexTriangles2D], 0,
                                    ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray());

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::TexTriangles2D], ezGALBufferHandle(),
                                                             &s_TexVertexDeclarationInfo, ezGALPrimitiveTopology::Triangles,
                                                             uiNum2DVerticesInBatch / 3);
          renderViewContext.m_pRenderContext->DrawMeshBuffer();

          uiNum2DVertices -= uiNum2DVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // Text
  {
    pData->m_glyphs.Clear();

    for (auto& textLine : pData->m_textLines3D)
    {
      ezVec3 screenPos;
      if (renderViewContext.m_pViewData->ComputeScreenSpacePos(textLine.m_position, screenPos).Succeeded() && screenPos.z > 0.0f)
      {
        textLine.m_topLeftCorner.x += ezMath::Round(screenPos.x);
        textLine.m_topLeftCorner.y += ezMath::Round(screenPos.y);

        AppendGlyphs(pData->m_glyphs, textLine);
      }
    }

    for (auto& textLine : pData->m_textLines2D)
    {
      AppendGlyphs(pData->m_glyphs, textLine);
    }


    ezUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);
      renderViewContext.m_pRenderContext->BindBuffer("glyphData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::Glyphs]));
      renderViewContext.m_pRenderContext->BindTexture2D("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        const ezUInt32 uiNumGlyphsInBatch = ezMath::Min<ezUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        pGALContext->UpdateBuffer(s_hDataBuffer[BufferType::Glyphs], 0, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr,
                                                           ezGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);
        renderViewContext.m_pRenderContext->DrawMeshBuffer();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

void ezDebugRenderer::OnEngineStartup()
{
  {
    ezGeometry geom;
    geom.AddLineBox(ezVec3(2.0f), ezColor::White);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Lines);

    s_hLineBoxMeshBuffer =
        ezResourceManager::CreateResource<ezMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    ezGeometry geom;
    geom.AddBox(ezVec3(2.0f), ezColor::White);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer =
        ezResourceManager::CreateResource<ezMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
  }

  {
    // reset, if already used before
    s_VertexDeclarationInfo.m_VertexStreams.Clear();

    {
      ezVertexStreamInfo& si = s_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Position;
      si.m_Format = ezGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      ezVertexStreamInfo& si = s_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Color;
      si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 4;
    }
  }

  {
    // reset, if already used before
    s_TexVertexDeclarationInfo.m_VertexStreams.Clear();

    {
      ezVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Position;
      si.m_Format = ezGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      ezVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Color;
      si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 4;
    }

    {
      ezVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
      si.m_Format = ezGALResourceFormat::XYFloat;
      si.m_uiOffset = 16;
      si.m_uiElementSize = 8;
    }

    {
      ezVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord1; // padding
      si.m_Format = ezGALResourceFormat::XYFloat;
      si.m_uiOffset = 24;
      si.m_uiElementSize = 8;
    }
  }

  {
    ezImage debugFontImage;
    ezGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);

    ezGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = debugFontImage.GetPixelPointer<ezUInt8>();
    memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(debugFontImage.GetDepthPitch());

    ezTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = debugFontImage.GetWidth();
    desc.m_DescGAL.m_uiHeight = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = ezResourceManager::CreateResource<ezTexture2DResource>("DebugFontTexture", std::move(desc));
  }

  s_hDebugGeometryShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugGeometry.ezShader");
  s_hDebugPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugPrimitive.ezShader");
  s_hDebugTexturedPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugTexturedPrimitive.ezShader");
  s_hDebugTextShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugText.ezShader");

  ezRenderWorld::s_EndRenderEvent.AddEventHandler(&ClearRenderData);
}

void ezDebugRenderer::OnEngineShutdown()
{
  ezRenderWorld::s_EndRenderEvent.RemoveEventHandler(&ClearRenderData);

  for (ezUInt32 i = 0; i < BufferType::Count; ++i)
  {
    DestroyBuffer(static_cast<BufferType::Enum>(i));
  }

  s_hLineBoxMeshBuffer.Invalidate();
  s_hSolidBoxMeshBuffer.Invalidate();
  s_hDebugFontTexture.Invalidate();

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
  s_hDebugTexturedPrimitiveShader.Invalidate();
  s_hDebugTextShader.Invalidate();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugRenderer);

