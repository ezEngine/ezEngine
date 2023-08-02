#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HybridArray.h>
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
  : m_uiId(pWorld != nullptr ? pWorld->GetIndex() : 0)
{
}

ezDebugRendererContext::ezDebugRendererContext(const ezViewHandle& hView)
  : m_uiId(hView.GetInternalID().m_Data)
{
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  struct alignas(16) Vertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(Vertex) == 16);

  struct alignas(16) TexVertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
    ezVec2 m_texCoord;
    float padding[2];
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(TexVertex) == 32);

  struct alignas(16) BoxData
  {
    ezShaderTransform m_transform;
    ezColor m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(BoxData) == 64);

  struct alignas(16) GlyphData
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

  struct InfoTextData
  {
    ezString m_group;
    ezString m_text;
    ezColor m_color;
  };

  struct PerContextData
  {
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_lineVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangleVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangle2DVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_line2DVertices;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_lineBoxes;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_solidBoxes;
    ezMap<ezGALResourceViewHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle2DVertices;
    ezMap<ezGALResourceViewHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle3DVertices;

    ezDynamicArray<InfoTextData> m_infoTextData[(int)ezDebugRenderer::ScreenPlacement::ENUM_COUNT];
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
    ezUniquePtr<PerContextData> m_pData[2];
  };

  static ezHashTable<ezDebugRendererContext, DoubleBufferedPerContextData> s_PerContextData;
  static ezMutex s_Mutex;

  static PerContextData& GetDataForExtraction(const ezDebugRendererContext& context)
  {
    DoubleBufferedPerContextData& doubleBufferedData = s_PerContextData[context];

    const ezUInt32 uiDataIndex = ezRenderWorld::IsRenderingThread() && (doubleBufferedData.m_uiLastRenderedFrame != ezRenderWorld::GetFrameCounter()) ? ezRenderWorld::GetDataIndexForRendering() : ezRenderWorld::GetDataIndexForExtraction();

    ezUniquePtr<PerContextData>& pData = doubleBufferedData.m_pData[uiDataIndex];
    if (pData == nullptr)
    {
      doubleBufferedData.m_pData[uiDataIndex] = EZ_DEFAULT_NEW(PerContextData);
    }

    return *pData;
  }

  static void ClearRenderData()
  {
    EZ_LOCK(s_Mutex);

    for (auto it = s_PerContextData.GetIterator(); it.IsValid(); ++it)
    {
      PerContextData* pData = it.Value().m_pData[ezRenderWorld::GetDataIndexForRendering()].Borrow();
      if (pData)
      {
        pData->m_lineVertices.Clear();
        pData->m_line2DVertices.Clear();
        pData->m_lineBoxes.Clear();
        pData->m_solidBoxes.Clear();
        pData->m_triangleVertices.Clear();
        pData->m_triangle2DVertices.Clear();
        pData->m_texTriangle2DVertices.Clear();
        pData->m_texTriangle3DVertices.Clear();
        pData->m_textLines2D.Clear();
        pData->m_textLines3D.Clear();

        for (ezUInt32 i = 0; i < (ezUInt32)ezDebugRenderer::ScreenPlacement::ENUM_COUNT; ++i)
        {
          pData->m_infoTextData[i].Clear();
        }
      }
    }
  }

  static void OnRenderEvent(const ezRenderWorldRenderEvent& e)
  {
    if (e.m_Type == ezRenderWorldRenderEvent::Type::EndRender)
    {
      ClearRenderData();
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
      TexTriangles3D,
      Glyphs,
      Lines2D,

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
  static ezUInt32 AddTextLines(const ezDebugRendererContext& context, const ezFormatString& text0, const ezVec2I32& vPositionInPixel, float fSizeInPixel, ezDebugRenderer::HorizontalAlignment horizontalAlignment, ezDebugRenderer::VerticalAlignment verticalAlignment, AddFunc func)
  {
    if (text0.IsEmpty())
      return 0;

    ezStringBuilder tmp;
    ezStringView text = text0.GetText(tmp);

    ezHybridArray<ezStringView, 8> lines;
    ezUInt32 maxLineLength = 0;

    ezHybridArray<ezUInt32, 8> maxColumWidth;
    bool isTabular = false;

    ezStringBuilder sb;
    if (text.FindSubString("\n"))
    {
      sb = text;
      sb.Split(true, lines, "\n");

      for (auto& line : lines)
      {
        ezUInt32 uiColIdx = 0;

        const char* colPtrCur = line.GetStartPointer();

        while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
        {
          isTabular = true;

          const ezUInt32 colLen = ezMath::RoundUp(1 + static_cast<ezUInt32>(colPtrNext - colPtrCur), 4);

          maxColumWidth.EnsureCount(uiColIdx + 1);
          maxColumWidth[uiColIdx] = ezMath::Max(maxColumWidth[uiColIdx], colLen);

          colPtrCur = colPtrNext + 1;
          ++uiColIdx;
        }

        // length of the last column (that wasn't counted)
        maxLineLength = ezMath::Max(maxLineLength, ezStringUtils::GetStringElementCount(colPtrCur, line.GetEndPointer()));
      }

      for (ezUInt32 columnWidth : maxColumWidth)
      {
        maxLineLength += columnWidth;
      }
    }
    else
    {
      lines.PushBack(text);
      maxLineLength = text.GetElementCount();
      maxColumWidth.PushBack(maxLineLength);
    }

    // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
    const float fGlyphWidth = ezMath::Ceil(fSizeInPixel * (8.0f / 16.0f));
    const float fLineHeight = ezMath::Ceil(fSizeInPixel * (20.0f / 16.0f));

    float screenPosX = (float)vPositionInPixel.x;
    if (horizontalAlignment == ezDebugRenderer::HorizontalAlignment::Right)
      screenPosX -= maxLineLength * fGlyphWidth;

    float screenPosY = (float)vPositionInPixel.y;
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

        if (isTabular)
        {
          ezUInt32 uiColIdx = 0;

          const char* colPtrCur = line.GetStartPointer();

          ezUInt32 addWidth = 0;

          while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
          {
            const ezVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, ezStringView(colPtrCur, colPtrNext), currentPos + tabOff);

            addWidth += maxColumWidth[uiColIdx];

            colPtrCur = colPtrNext + 1;
            ++uiColIdx;
          }

          // last column
          {
            const ezVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, ezStringView(colPtrCur, line.GetEndPointer()), currentPos + tabOff);
          }
        }
        else
        {
          func(data, line, currentPos);
        }

        currentPos.y += fLineHeight;
      }
    }

    return lines.GetCount();
  }

  static void AppendGlyphs(ezDynamicArray<GlyphData, ezAlignedAllocatorWrapper>& ref_glyphs, const TextLineData2D& textLine)
  {
    ezVec2 currentPos = textLine.m_topLeftCorner;
    const float fGlyphWidth = ezMath::Ceil(textLine.m_uiSizeInPixel * (8.0f / 16.0f));

    for (ezUInt32 uiCharacter : textLine.m_text)
    {
      auto& glyphData = ref_glyphs.ExpandAndGetRef();
      glyphData.m_topLeftCorner = currentPos;
      glyphData.m_color = textLine.m_color;
      glyphData.m_glyphIndex = uiCharacter < 128 ? static_cast<ezUInt16>(uiCharacter) : 0;
      glyphData.m_sizeInPixel = (ezUInt16)textLine.m_uiSizeInPixel;

      currentPos.x += fGlyphWidth;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Persistent Items

  struct PersistentCrossData
  {
    float m_fSize;
    ezColor m_Color;
    ezTransform m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentSphereData
  {
    float m_fRadius;
    ezColor m_Color;
    ezTransform m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentBoxData
  {
    ezVec3 m_vHalfSize;
    ezColor m_Color;
    ezTransform m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentPerContextData
  {
    ezTime m_Now;
    ezDeque<PersistentCrossData> m_Crosses;
    ezDeque<PersistentSphereData> m_Spheres;
    ezDeque<PersistentBoxData> m_Boxes;
  };

  static ezHashTable<ezDebugRendererContext, PersistentPerContextData> s_PersistentPerContextData;

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
void ezDebugRenderer::DrawLines(const ezDebugRendererContext& context, ezArrayPtr<const Line> lines, const ezColor& color, const ezTransform& transform /*= ezTransform::IdentityTransform()*/)
{
  if (lines.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const ezVec3* pPositions = &line.m_start;
    const ezColor* pColors = &line.m_startColor;

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_lineVertices.ExpandAndGetRef();
      vertex.m_position = transform.TransformPosition(pPositions[i]);
      vertex.m_color = pColors[i] * color;
    }
  }
}

void ezDebugRenderer::Draw2DLines(const ezDebugRendererContext& context, ezArrayPtr<const Line> lines, const ezColor& color)
{
  if (lines.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const ezVec3* pPositions = &line.m_start;

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_line2DVertices.ExpandAndGetRef();
      vertex.m_position = pPositions[i];
      vertex.m_color = color;
    }
  }
}

// static
void ezDebugRenderer::DrawCross(const ezDebugRendererContext& context, const ezVec3& vGlobalPosition, float fLineLength, const ezColor& color, const ezTransform& transform /*= ezTransform::IdentityTransform()*/)
{
  if (fLineLength <= 0.0f)
    return;

  const float fHalfLineLength = fLineLength * 0.5f;
  const ezVec3 xAxis = ezVec3::UnitXAxis() * fHalfLineLength;
  const ezVec3 yAxis = ezVec3::UnitYAxis() * fHalfLineLength;
  const ezVec3 zAxis = ezVec3::UnitZAxis() * fHalfLineLength;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - xAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + xAxis), color});

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - yAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + yAxis), color});

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - zAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + zAxis), color});
}

// static
void ezDebugRenderer::DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();

  ezTransform boxTransform(box.GetCenter(), ezQuat::IdentityQuaternion(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform;
  boxData.m_color = color;
}

// static
void ezDebugRenderer::DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform)
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
void ezDebugRenderer::DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color, const ezTransform& transform /*= ezTransform::IdentityTransform()*/)
{
  enum
  {
    NUM_SEGMENTS = 32
  };

  const ezVec3 vCenter = sphere.m_vCenter;
  const float fRadius = sphere.m_fRadius;
  const ezAngle stepAngle = ezAngle::MakeFromDegree(360.0f / NUM_SEGMENTS);

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (ezUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = ezMath::Cos(fS1 * stepAngle);
    const float fCos2 = ezMath::Cos(fS2 * stepAngle);

    const float fSin1 = ezMath::Sin(fS1 * stepAngle);
    const float fSin2 = ezMath::Sin(fS2 * stepAngle);

    data.m_lineVertices.PushBack({transform * (vCenter + ezVec3(0.0f, fCos1, fSin1) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + ezVec3(0.0f, fCos2, fSin2) * fRadius), color});

    data.m_lineVertices.PushBack({transform * (vCenter + ezVec3(fCos1, 0.0f, fSin1) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + ezVec3(fCos2, 0.0f, fSin2) * fRadius), color});

    data.m_lineVertices.PushBack({transform * (vCenter + ezVec3(fCos1, fSin1, 0.0f) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + ezVec3(fCos2, fSin2, 0.0f) * fRadius), color});
  }
}


void ezDebugRenderer::DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, const ezTransform& transform /*= ezTransform::IdentityTransform()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const ezAngle stepAngle = ezAngle::MakeFromDegree(360.0f / NUM_SEGMENTS);

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
void ezDebugRenderer::DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color, bool bDrawPlaneNormals /*= false*/)
{
  ezVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints);

  Line lines[12] = {
    Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft]),
    Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight]),
    Line(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft]),
    Line(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight]),

    Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight]),
    Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight]),
    Line(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft]),
    Line(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft]),

    Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight]),
    Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight]),
    Line(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft]),
    Line(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft]),
  };

  DrawLines(context, lines, color);

  if (bDrawPlaneNormals)
  {
    ezColor normalColor = color + ezColor(0.4f, 0.4f, 0.4f);
    float fDrawLength = 0.5f;

    const ezVec3 nearPlaneNormal = frustum.GetPlane(0).m_vNormal * fDrawLength;
    const ezVec3 farPlaneNormal = frustum.GetPlane(1).m_vNormal * fDrawLength;
    const ezVec3 leftPlaneNormal = frustum.GetPlane(2).m_vNormal * fDrawLength;
    const ezVec3 rightPlaneNormal = frustum.GetPlane(3).m_vNormal * fDrawLength;
    const ezVec3 bottomPlaneNormal = frustum.GetPlane(4).m_vNormal * fDrawLength;
    const ezVec3 topPlaneNormal = frustum.GetPlane(5).m_vNormal * fDrawLength;

    Line normalLines[24] = {
      Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft] + nearPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight] + nearPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft] + nearPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight] + nearPlaneNormal),

      Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft] + farPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight] + farPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft] + farPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight] + farPlaneNormal),

      Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft] + leftPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft] + leftPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft] + leftPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft] + leftPlaneNormal),

      Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight] + rightPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight] + rightPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight] + rightPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight] + rightPlaneNormal),

      Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft] + bottomPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight] + bottomPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft] + bottomPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight] + bottomPlaneNormal),

      Line(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft] + topPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight] + topPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft] + topPlaneNormal),
      Line(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight] + topPlaneNormal),
    };

    DrawLines(context, normalLines, normalColor);
  }
}

// static
void ezDebugRenderer::DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform)
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
    const ezColorLinearUB col = triangle.m_color * color;

    for (ezUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.m_triangleVertices.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_color = col;
    }
  }
}

void ezDebugRenderer::DrawTexturedTriangles(const ezDebugRendererContext& context, ezArrayPtr<TexturedTriangle> triangles, const ezColor& color, const ezTexture2DResourceHandle& hTexture)
{
  if (triangles.IsEmpty())
    return;

  ezResourceLock<ezTexture2DResource> pTexture(hTexture, ezResourceAcquireMode::AllowLoadingFallback);
  auto hResourceView = ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture());

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context).m_texTriangle3DVertices[hResourceView];

  for (auto& triangle : triangles)
  {
    const ezColorLinearUB col = triangle.m_color * color;

    for (ezUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_texCoord = triangle.m_texcoord[i];
      vertex.m_color = col;
    }
  }
}

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color)
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

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, const ezTexture2DResourceHandle& hTexture, ezVec2 vScale)
{
  ezResourceLock<ezTexture2DResource> pTexture(hTexture, ezResourceAcquireMode::AllowLoadingFallback);
  Draw2DRectangle(context, rectInPixel, fDepth, color, ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()), vScale);
}

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, ezGALResourceViewHandle hResourceView, ezVec2 vScale)
{
  TexVertex vertices[6];

  vertices[0].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[0].m_texCoord = ezVec2(0, 0).CompMul(vScale);
  vertices[1].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[1].m_texCoord = ezVec2(1, 1).CompMul(vScale);
  vertices[2].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_texCoord = ezVec2(0, 1).CompMul(vScale);
  vertices[3].m_position = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[3].m_texCoord = ezVec2(0, 0).CompMul(vScale);
  vertices[4].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[4].m_texCoord = ezVec2(1, 0).CompMul(vScale);
  vertices[5].m_position = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[5].m_texCoord = ezVec2(1, 1).CompMul(vScale);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_texTriangle2DVertices[hResourceView].PushBackRange(ezMakeArrayPtr(vertices));
}

ezUInt32 ezDebugRenderer::Draw2DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec2I32& vPositionInPixel, const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/, HorizontalAlignment horizontalAlignment /*= HorizontalAlignment::Left*/, VerticalAlignment verticalAlignment /*= VerticalAlignment::Top*/)
{
  return AddTextLines(context, text, vPositionInPixel, (float)uiSizeInPixel, horizontalAlignment, verticalAlignment, [=](PerContextData& ref_data, ezStringView sLine, ezVec2 vTopLeftCorner) {
    auto& textLine = ref_data.m_textLines2D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel;
  });
}


void ezDebugRenderer::DrawInfoText(const ezDebugRendererContext& context, ScreenPlacement placement, const char* szGroupName, const ezFormatString& text, const ezColor& color)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  ezStringBuilder tmp;

  auto& e = data.m_infoTextData[(int)placement].ExpandAndGetRef();
  e.m_group = szGroupName;
  e.m_text = text.GetText(tmp);
  e.m_color = color;
}

ezUInt32 ezDebugRenderer::Draw3DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec3& vGlobalPosition, const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/, HorizontalAlignment horizontalAlignment /*= HorizontalAlignment::Center*/, VerticalAlignment verticalAlignment /*= VerticalAlignment::Bottom*/)
{
  return AddTextLines(context, text, ezVec2I32(0), (float)uiSizeInPixel, horizontalAlignment, verticalAlignment, [&](PerContextData& ref_data, ezStringView sLine, ezVec2 vTopLeftCorner) {
    auto& textLine = ref_data.m_textLines3D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel;
    textLine.m_position = vGlobalPosition;
  });
}

void ezDebugRenderer::AddPersistentCross(const ezDebugRendererContext& context, float fSize, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Crosses.ExpandAndGetRef();
  item.m_Transform = transform;
  item.m_Color = color;
  item.m_fSize = fSize;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::AddPersistentLineSphere(const ezDebugRendererContext& context, float fRadius, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Spheres.ExpandAndGetRef();
  item.m_Transform = transform;
  item.m_Color = color;
  item.m_fRadius = fRadius;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::AddPersistentLineBox(const ezDebugRendererContext& context, const ezVec3& vHalfSize, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Boxes.ExpandAndGetRef();
  item.m_Transform = transform;
  item.m_Color = color;
  item.m_vHalfSize = vHalfSize;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::DrawAngle(const ezDebugRendererContext& context, ezAngle startAngle, ezAngle endAngle, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, ezVec3 vForwardAxis /*= ezVec3::UnitXAxis()*/, ezVec3 vRotationAxis /*= ezVec3::UnitZAxis()*/)
{
  ezHybridArray<Triangle, 64> tris;
  ezHybridArray<Line, 64> lines;

  startAngle.NormalizeRange();
  endAngle.NormalizeRange();

  if (startAngle > endAngle)
    startAngle -= ezAngle::MakeFromDegree(360);

  const ezAngle range = endAngle - startAngle;
  const ezUInt32 uiTesselation = ezMath::Max(1u, (ezUInt32)(range / ezAngle::MakeFromDegree(5)));
  const ezAngle step = range / (float)uiTesselation;

  ezQuat qStart;
  qStart.SetFromAxisAndAngle(vRotationAxis, startAngle);

  ezQuat qStep;
  qStep.SetFromAxisAndAngle(vRotationAxis, step);

  ezVec3 vCurDir = qStart * vForwardAxis;

  if (lineColor.a > 0)
  {
    Line& l1 = lines.ExpandAndGetRef();
    l1.m_start.SetZero();
    l1.m_end = vCurDir;
  }

  for (ezUInt32 i = 0; i < uiTesselation; ++i)
  {
    const ezVec3 vNextDir = qStep * vCurDir;

    if (solidColor.a > 0)
    {
      Triangle& tri1 = tris.ExpandAndGetRef();
      tri1.m_position[0] = transform.m_vPosition;
      tri1.m_position[1] = transform.TransformPosition(vNextDir);
      tri1.m_position[2] = transform.TransformPosition(vCurDir);

      Triangle& tri2 = tris.ExpandAndGetRef();
      tri2.m_position[0] = transform.m_vPosition;
      tri2.m_position[1] = transform.TransformPosition(vCurDir);
      tri2.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (lineColor.a > 0)
    {
      Line& l1 = lines.ExpandAndGetRef();
      l1.m_start.SetZero();
      l1.m_end = vNextDir;

      Line& l2 = lines.ExpandAndGetRef();
      l2.m_start = vCurDir;
      l2.m_end = vNextDir;
    }

    vCurDir = vNextDir;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void ezDebugRenderer::DrawOpeningCone(const ezDebugRendererContext& context, ezAngle halfAngle, const ezColor& colorInside, const ezColor& colorOutside, const ezTransform& transform, ezVec3 vForwardAxis /*= ezVec3::UnitXAxis()*/)
{
  ezHybridArray<Triangle, 64> trisInside;
  ezHybridArray<Triangle, 64> trisOutside;

  halfAngle = ezMath::Clamp(halfAngle, ezAngle(), ezAngle::MakeFromDegree(180));

  const ezAngle refAngle = halfAngle <= ezAngle::MakeFromDegree(90) ? halfAngle : ezAngle::MakeFromDegree(180) - halfAngle;
  const ezUInt32 uiTesselation = ezMath::Max(8u, (ezUInt32)(refAngle / ezAngle::MakeFromDegree(2)));

  const ezVec3 tangentAxis = vForwardAxis.GetOrthogonalVector().GetNormalized();

  ezQuat tilt;
  tilt.SetFromAxisAndAngle(tangentAxis, halfAngle);

  ezQuat step;
  step.SetFromAxisAndAngle(vForwardAxis, ezAngle::MakeFromDegree(360) / (float)uiTesselation);

  ezVec3 vCurDir = tilt * vForwardAxis;

  for (ezUInt32 i = 0; i < uiTesselation; ++i)
  {
    const ezVec3 vNextDir = step * vCurDir;

    if (colorInside.a > 0)
    {
      Triangle& tri = trisInside.ExpandAndGetRef();
      tri.m_position[0] = transform.m_vPosition;
      tri.m_position[1] = transform.TransformPosition(vCurDir);
      tri.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (colorOutside.a > 0)
    {
      Triangle& tri = trisOutside.ExpandAndGetRef();
      tri.m_position[0] = transform.m_vPosition;
      tri.m_position[1] = transform.TransformPosition(vNextDir);
      tri.m_position[2] = transform.TransformPosition(vCurDir);
    }

    vCurDir = vNextDir;
  }


  DrawSolidTriangles(context, trisInside, colorInside);
  DrawSolidTriangles(context, trisOutside, colorOutside);
}

void ezDebugRenderer::DrawLimitCone(const ezDebugRendererContext& context, ezAngle halfAngle1, ezAngle halfAngle2, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform)
{
  constexpr ezUInt32 NUM_LINES = 32;
  ezHybridArray<Line, NUM_LINES * 2> lines;
  ezHybridArray<Triangle, NUM_LINES * 2> tris;

  // no clue how this works
  // copied 1:1 from NVIDIA's PhysX SDK: Cm::visualizeLimitCone
  {
    float scale = 1.0f;

    const float tanQSwingZ = ezMath::Tan(halfAngle1 / 4);
    const float tanQSwingY = ezMath::Tan(halfAngle2 / 4);

    ezVec3 prev(0);
    for (ezUInt32 i = 0; i <= NUM_LINES; i++)
    {
      const float angle = 2 * ezMath::Pi<float>() / NUM_LINES * i;
      const float c = ezMath::Cos(ezAngle::MakeFromRadian(angle)), s = ezMath::Sin(ezAngle::MakeFromRadian(angle));
      const ezVec3 rv(0, -tanQSwingZ * s, tanQSwingY * c);
      const float rv2 = rv.GetLengthSquared();
      const float r = (1 / (1 + rv2));
      const ezQuat q = ezQuat(0, r * 2 * rv.y, r * 2 * rv.z, r * (1 - rv2));
      const ezVec3 a = q * ezVec3(1.0f, 0, 0) * scale;

      if (lineColor.a > 0)
      {
        auto& l1 = lines.ExpandAndGetRef();
        l1.m_start = prev;
        l1.m_end = a;

        auto& l2 = lines.ExpandAndGetRef();
        l2.m_start.SetZero();
        l2.m_end = a;
      }

      if (solidColor.a > 0)
      {
        auto& t1 = tris.ExpandAndGetRef();
        t1.m_position[0] = transform.m_vPosition;
        t1.m_position[1] = transform.TransformPosition(prev);
        t1.m_position[2] = transform.TransformPosition(a);

        auto& t2 = tris.ExpandAndGetRef();
        t2.m_position[0] = transform.m_vPosition;
        t2.m_position[1] = transform.TransformPosition(a);
        t2.m_position[2] = transform.TransformPosition(prev);
      }

      prev = a;
    }
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void ezDebugRenderer::DrawCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, bool bCapStart /*= false*/, bool bCapEnd /*= false*/)
{
  constexpr ezUInt32 NUM_SEGMENTS = 16;
  ezHybridArray<Line, NUM_SEGMENTS * 3> lines;
  ezHybridArray<Triangle, NUM_SEGMENTS * 2 * 2> tris;

  const ezAngle step = ezAngle::MakeFromDegree(360) / NUM_SEGMENTS;
  ezAngle angle = {};

  ezVec3 vCurCircle(0, 1 /*ezMath::Cos(angle)*/, 0 /*ezMath::Sin(angle)*/);

  const bool bSolid = solidColor.a > 0;
  const bool bLine = lineColor.a > 0;

  const ezVec3 vLastCircle(0, ezMath::Cos(-step), ezMath::Sin(-step));
  const ezVec3 vLastStart = transform.TransformPosition(ezVec3(0, vLastCircle.y * fRadiusStart, vLastCircle.z * fRadiusStart));
  const ezVec3 vLastEnd = transform.TransformPosition(ezVec3(fLength, vLastCircle.y * fRadiusEnd, vLastCircle.z * fRadiusEnd));

  for (ezUInt32 i = 0; i < NUM_SEGMENTS; ++i)
  {
    angle += step;
    const ezVec3 vNextCircle(0, ezMath::Cos(angle), ezMath::Sin(angle));

    ezVec3 vCurStart = vCurCircle * fRadiusStart;
    ezVec3 vNextStart = vNextCircle * fRadiusStart;

    ezVec3 vCurEnd(fLength, vCurCircle.y * fRadiusEnd, vCurCircle.z * fRadiusEnd);
    ezVec3 vNextEnd(fLength, vNextCircle.y * fRadiusEnd, vNextCircle.z * fRadiusEnd);

    if (bLine)
    {
      lines.PushBack({vCurStart, vNextStart});
      lines.PushBack({vCurEnd, vNextEnd});
      lines.PushBack({vCurStart, vCurEnd});
    }

    if (bSolid)
    {
      vCurStart = transform.TransformPosition(vCurStart);
      vCurEnd = transform.TransformPosition(vCurEnd);
      vNextStart = transform.TransformPosition(vNextStart);
      vNextEnd = transform.TransformPosition(vNextEnd);

      tris.PushBack({vCurStart, vNextStart, vNextEnd});
      tris.PushBack({vCurStart, vNextEnd, vCurEnd});

      if (bCapStart)
        tris.PushBack({vLastStart, vNextStart, vCurStart});

      if (bCapEnd)
        tris.PushBack({vLastEnd, vCurEnd, vNextEnd});
    }

    vCurCircle = vNextCircle;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
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
  {
    EZ_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = ezTime::Now();

    // persistent crosses
    {
      ezUInt32 uiNumItems = data.m_Crosses.GetCount();
      for (ezUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Crosses[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Crosses.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          ezDebugRenderer::DrawCross(context, ezVec3::ZeroVector(), item.m_fSize, item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent spheres
    {
      ezUInt32 uiNumItems = data.m_Spheres.GetCount();
      for (ezUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Spheres[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Spheres.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          ezDebugRenderer::DrawLineSphere(context, ezBoundingSphere(ezVec3::ZeroVector(), item.m_fRadius), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent boxes
    {
      ezUInt32 uiNumItems = data.m_Boxes.GetCount();
      for (ezUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Boxes[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Boxes.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          ezDebugRenderer::DrawLineBox(context, ezBoundingBox(-item.m_vHalfSize, item.m_vHalfSize), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }
  }

  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
  {
    return;
  }

  PerContextData* pData = pDoubleBufferedContextData->m_pData[ezRenderWorld::GetDataIndexForRendering()].Borrow();
  if (pData == nullptr)
  {
    return;
  }

  // draw info text
  {
    static_assert((int)ezDebugRenderer::ScreenPlacement::ENUM_COUNT == 6);

    HorizontalAlignment ha[(int)ezDebugRenderer::ScreenPlacement::ENUM_COUNT] = {
      HorizontalAlignment::Left,
      HorizontalAlignment::Center,
      HorizontalAlignment::Right,
      HorizontalAlignment::Left,
      HorizontalAlignment::Center,
      HorizontalAlignment::Right};

    VerticalAlignment va[(int)ezDebugRenderer::ScreenPlacement::ENUM_COUNT] = {
      VerticalAlignment::Top,
      VerticalAlignment::Top,
      VerticalAlignment::Top,
      VerticalAlignment::Bottom,
      VerticalAlignment::Bottom,
      VerticalAlignment::Bottom};

    int offs[(int)ezDebugRenderer::ScreenPlacement::ENUM_COUNT] = {20, 20, 20, -20, -20, -20};

    ezInt32 resX = (ezInt32)renderViewContext.m_pViewData->m_ViewPortRect.width;
    ezInt32 resY = (ezInt32)renderViewContext.m_pViewData->m_ViewPortRect.height;

    ezVec2I32 anchor[(int)ezDebugRenderer::ScreenPlacement::ENUM_COUNT] = {
      ezVec2I32(10, 10),
      ezVec2I32(resX / 2, 10),
      ezVec2I32(resX - 10, 10),
      ezVec2I32(10, resY - 10),
      ezVec2I32(resX / 2, resY - 10),
      ezVec2I32(resX - 10, resY - 10)};

    for (ezUInt32 corner = 0; corner < (ezUInt32)ezDebugRenderer::ScreenPlacement::ENUM_COUNT; ++corner)
    {
      auto& cd = pData->m_infoTextData[corner];

      // InsertionSort is stable
      ezSorting::InsertionSort(cd, [](const InfoTextData& lhs, const InfoTextData& rhs) -> bool { return lhs.m_group < rhs.m_group; });

      ezVec2I32 pos = anchor[corner];

      for (ezUInt32 i = 0; i < cd.GetCount(); ++i)
      {
        // add some space between groups
        if (i > 0 && cd[i - 1].m_group != cd[i].m_group)
          pos.y += offs[corner];

        pos.y += offs[corner] * Draw2DText(context, cd[i].m_text.GetData(), pos, cd[i].m_color, 16, ha[corner], va[corner]);
      }
    }
  }

  // update the frame counter
  pDoubleBufferedContextData->m_uiLastRenderedFrame = ezRenderWorld::GetFrameCounter();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

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
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::SolidBoxes], 0, ezMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch).ToByteArray());

        unsigned int uiRenderedInstances = uiNumSolidBoxesInBatch;
        if (renderViewContext.m_pCamera->IsStereoscopic())
          uiRenderedInstances *= 2;

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiRenderedInstances).IgnoreResult();

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
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Triangles3D], 0, ezMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles3D], ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNumTriangleVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumTriangleVertices -= uiNumTriangleVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 3D triangles
  {
    for (auto itTex = pData->m_texTriangle3DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", itTex.Key());

      const auto& verts = itTex.Value();

      ezUInt32 uiNumVertices = verts.GetCount();
      if (uiNumVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles3D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNumVertices > 0)
        {
          const ezUInt32 uiNumVerticesInBatch = ezMath::Min<ezUInt32>(uiNumVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          EZ_ASSERT_DEV(uiNumVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::TexTriangles3D], 0, ezMakeArrayPtr(pTriangleData, uiNumVerticesInBatch).ToByteArray());

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::TexTriangles3D], ezGALBufferHandle(), &s_TexVertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNumVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNumVertices -= uiNumVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // 3D Lines
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
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Lines], 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Lines], ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // 2D Lines
  {
    ezUInt32 uiNumLineVertices = pData->m_line2DVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines2D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_line2DVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Lines2D], 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Lines2D], ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

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
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::LineBoxes], 0, ezMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch).IgnoreResult();

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
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Triangles2D], 0, ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles2D], ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 2D triangles
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
          pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::TexTriangles2D], 0, ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray());

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::TexTriangles2D], ezGALBufferHandle(), &s_TexVertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

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
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Glyphs], 0, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

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
    geom.AddLineBox(ezVec3(2.0f));

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Lines);

    s_hLineBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    ezGeometry geom;
    geom.AddBox(ezVec3(2.0f), false);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
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
      si.m_Semantic = ezGALVertexAttributeSemantic::Color0;
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
      si.m_Semantic = ezGALVertexAttributeSemantic::Color0;
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

  ezRenderWorld::GetRenderEvent().AddEventHandler(&OnRenderEvent);
}

void ezDebugRenderer::OnEngineShutdown()
{
  ezRenderWorld::GetRenderEvent().RemoveEventHandler(&OnRenderEvent);

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

  s_PerContextData.Clear();

  s_PersistentPerContextData.Clear();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugRenderer);
