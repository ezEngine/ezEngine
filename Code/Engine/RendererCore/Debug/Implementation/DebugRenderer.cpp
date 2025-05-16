#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/SimpleASCIIFont.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/BufferPool.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Shader/Types.h>

ezCVarFloat cvar_DebugTextScale("Debug.TextScale", 1.0f, ezCVarFlags::Save, "Global scale for debug text");

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

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDebugTextHAlign, 1)
  EZ_ENUM_CONSTANTS(ezDebugTextHAlign::Left, ezDebugTextHAlign::Center, ezDebugTextHAlign::Right)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDebugTextVAlign, 1)
  EZ_ENUM_CONSTANTS(ezDebugTextVAlign::Top, ezDebugTextVAlign::Center, ezDebugTextVAlign::Bottom)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDebugTextPlacement, 1)
  EZ_ENUM_CONSTANTS(ezDebugTextPlacement::TopLeft, ezDebugTextPlacement::TopCenter, ezDebugTextPlacement::TopRight)
  EZ_ENUM_CONSTANTS(ezDebugTextPlacement::BottomLeft, ezDebugTextPlacement::BottomCenter, ezDebugTextPlacement::BottomRight)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

namespace
{
  struct alignas(16) Vertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
  };

  static_assert(sizeof(Vertex) == 16);

  struct alignas(16) TexVertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
    ezVec2 m_texCoord;
    float padding[2];
  };

  static_assert(sizeof(TexVertex) == 32);

  struct alignas(16) BoxData
  {
    ezShaderTransform m_transform;
    ezColor m_color;
  };

  static_assert(sizeof(BoxData) == 64);

  struct alignas(16) GlyphData
  {
    ezVec2 m_topLeftCorner;
    ezColorLinearUB m_color;
    ezUInt16 m_glyphIndex;
    ezUInt16 m_sizeInPixel;
  };

  static_assert(sizeof(GlyphData) == 16);

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
    ezMap<ezGALTextureResourceViewHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle2DVertices;
    ezMap<ezGALTextureResourceViewHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle3DVertices;

    ezDynamicArray<InfoTextData> m_infoTextData[(int)ezDebugTextPlacement::ENUM_COUNT];
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

        for (ezUInt32 i = 0; i < (ezUInt32)ezDebugTextPlacement::ENUM_COUNT; ++i)
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

  static ezGALBufferPool s_DataBuffer[BufferType::Count];

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
    if (!s_DataBuffer[bufferType].IsInitialized())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiStructSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::Transient;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_DataBuffer[bufferType].Initialize(desc, "DebugRenderer - StructuredBuffer");
    }
  }

  static void CreateVertexBuffer(BufferType::Enum bufferType, ezUInt32 uiVertexSize)
  {
    if (!s_DataBuffer[bufferType].IsInitialized())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiVertexSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer | ezGALBufferUsageFlags::Transient;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_DataBuffer[bufferType].Initialize(desc, "DebugRenderer - VertexBuffer");
    }
  }

  static void DestroyBuffer(BufferType::Enum bufferType)
  {
    s_DataBuffer[bufferType].Deinitialize();
  }

  template <typename AddFunc>
  static ezUInt32 AddTextLines(const ezDebugRendererContext& context, const ezFormatString& text0, const ezVec2I32& vPositionInPixel, ezUInt32 uiSizeInPixel, ezDebugTextHAlign::Enum horizontalAlignment, ezDebugTextVAlign::Enum verticalAlignment, AddFunc func)
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


    const float fGlyphWidth = ezDebugRenderer::GetTextGlyphWidth(uiSizeInPixel);
    const float fGlyphHeight = ezMath::Ceil(uiSizeInPixel * cvar_DebugTextScale);
    const float fLineHeight = ezDebugRenderer::GetTextLineHeight(uiSizeInPixel);
    const float fLineSpacing = fLineHeight - fGlyphHeight;

    float screenPosX = (float)vPositionInPixel.x;
    if (horizontalAlignment == ezDebugTextHAlign::Right)
      screenPosX -= maxLineLength * fGlyphWidth;

    float screenPosY = (float)vPositionInPixel.y;
    if (verticalAlignment == ezDebugTextVAlign::Center)
      screenPosY -= ezMath::Ceil(lines.GetCount() * fLineHeight * 0.5f) - fLineSpacing * 0.5f;
    else if (verticalAlignment == ezDebugTextVAlign::Bottom)
      screenPosY -= lines.GetCount() * fLineHeight - fLineSpacing;

    {
      EZ_LOCK(s_Mutex);

      auto& data = GetDataForExtraction(context);

      ezVec2 currentPos(screenPosX, screenPosY);

      for (ezStringView line : lines)
      {
        currentPos.x = screenPosX;
        if (horizontalAlignment == ezDebugTextHAlign::Center)
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
    const float fGlyphWidth = ezDebugRenderer::GetTextGlyphWidth(textLine.m_uiSizeInPixel);

    for (ezUInt32 uiCharacter : textLine.m_text)
    {
      auto& glyphData = ref_glyphs.ExpandAndGetRef();
      glyphData.m_topLeftCorner = currentPos;
      glyphData.m_color = textLine.m_color;
      glyphData.m_glyphIndex = uiCharacter < 128 ? static_cast<ezUInt16>(uiCharacter) : 0;
      glyphData.m_sizeInPixel = (ezUInt16)ezMath::Ceil(textLine.m_uiSizeInPixel * cvar_DebugTextScale);

      currentPos.x += fGlyphWidth;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Persistent Items

  struct PersistentCrossData
  {
    float m_fSize;
    ezColor m_Color;
    ezMat4 m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentSphereData
  {
    float m_fRadius;
    ezColor m_Color;
    ezMat4 m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentBoxData
  {
    ezVec3 m_vHalfSize;
    ezColor m_Color;
    ezMat4 m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentLineData
  {
    ezHybridArray<ezDebugRendererLine, 32> m_Lines;
    ezColor m_Color;
    ezMat4 m_Transform;
    ezTime m_Timeout;
  };

  struct PersistentInfoTextData
  {
    ezString m_sText;
    ezDebugTextPlacement::Enum m_Placement;
    ezColor m_Color;
    ezTime m_Timeout;
  };

  struct PersistentPerContextData
  {
    ezTime m_Now;
    ezDeque<PersistentCrossData> m_Crosses;
    ezDeque<PersistentSphereData> m_Spheres;
    ezDeque<PersistentBoxData> m_Boxes;
    ezDeque<PersistentLineData> m_Lines;
    ezDeque<PersistentInfoTextData> m_InfoText;
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
void ezDebugRenderer::DrawLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
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
      vertex.m_position = mTransform.m_Mat4.TransformPosition(pPositions[i]);
      vertex.m_color = pColors[i] * color;
    }
  }
}

void ezDebugRenderer::Draw2DLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color)
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
void ezDebugRenderer::DrawCross(const ezDebugRendererContext& context, const ezVec3& vGlobalPosition, float fLineLength, const ezColor& color, ezMatOrTransform mTransform0 /*= ezMat4::MakeIdentity()*/)
{
  if (fLineLength <= 0.0f)
    return;

  const ezMat4& transform = mTransform0.m_Mat4;

  const float fHalfLineLength = fLineLength * 0.5f;
  const ezVec3 xAxis = ezVec3::MakeAxisX() * fHalfLineLength;
  const ezVec3 yAxis = ezVec3::MakeAxisY() * fHalfLineLength;
  const ezVec3 zAxis = ezVec3::MakeAxisZ() * fHalfLineLength;

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
void ezDebugRenderer::DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, ezMatOrTransform mTransform0)
{
  EZ_LOCK(s_Mutex);

  const ezMat4& transform = mTransform0.m_Mat4;

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();

  ezTransform boxTransform(box.GetCenter(), ezQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform.GetAsMat4();
  boxData.m_color = color;
}

// static
void ezDebugRenderer::DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, ezMatOrTransform mTransform0)
{
  const ezMat4& transform = mTransform0.m_Mat4;

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

  ezDebugRendererLine lines[24];
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
void ezDebugRenderer::DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color, ezMatOrTransform mTransform0 /*= ezMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32
  };

  const ezVec3 vCenter = sphere.m_vCenter;
  const float fRadius = sphere.m_fRadius;
  const ezAngle stepAngle = ezAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  const ezMat4& transform = mTransform0.m_Mat4;

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


void ezDebugRenderer::DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, ezMatOrTransform mTransform0 /*= ezMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const ezMat4& transform = mTransform0.m_Mat4;

  const ezAngle stepAngle = ezAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  ezDebugRendererLine lines[NUM_LINES];

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

void ezDebugRenderer::DrawLineCylinderZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, ezMatOrTransform mTransform0 /*= ezMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const ezMat4& transform = mTransform0.m_Mat4;

  const ezAngle stepAngle = ezAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  ezDebugRendererLine lines[NUM_LINES];

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

  EZ_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

// static
void ezDebugRenderer::DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color, bool bDrawPlaneNormals /*= false*/)
{
  ezVec3 cornerPoints[8];
  if (frustum.ComputeCornerPoints(cornerPoints).Failed())
    return;

  ezDebugRendererLine lines[12] = {
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight]),

    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft]),

    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft]),
    ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft]),
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

    ezDebugRendererLine normalLines[24] = {
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft] + nearPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight] + nearPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft] + nearPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight] + nearPlaneNormal),

      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft] + farPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight] + farPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft] + farPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight] + farPlaneNormal),

      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft] + leftPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft] + leftPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft] + leftPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft] + leftPlaneNormal),

      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight] + rightPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight] + rightPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight] + rightPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight] + rightPlaneNormal),

      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft], cornerPoints[ezFrustum::FrustumCorner::NearBottomLeft] + bottomPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearBottomRight], cornerPoints[ezFrustum::FrustumCorner::NearBottomRight] + bottomPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft], cornerPoints[ezFrustum::FrustumCorner::FarBottomLeft] + bottomPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarBottomRight], cornerPoints[ezFrustum::FrustumCorner::FarBottomRight] + bottomPlaneNormal),

      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopLeft], cornerPoints[ezFrustum::FrustumCorner::NearTopLeft] + topPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::NearTopRight], cornerPoints[ezFrustum::FrustumCorner::NearTopRight] + topPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopLeft], cornerPoints[ezFrustum::FrustumCorner::FarTopLeft] + topPlaneNormal),
      ezDebugRendererLine(cornerPoints[ezFrustum::FrustumCorner::FarTopRight], cornerPoints[ezFrustum::FrustumCorner::FarTopRight] + topPlaneNormal),
    };

    DrawLines(context, normalLines, normalColor);
  }
}

// static
void ezDebugRenderer::DrawSolidBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, ezMatOrTransform mTransform0)
{
  const ezMat4& transform = mTransform0.m_Mat4;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_solidBoxes.ExpandAndGetRef();

  ezTransform boxTransform(box.GetCenter(), ezQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform.GetAsMat4();
  boxData.m_color = color;
}

// static
void ezDebugRenderer::DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<ezDebugRendererTriangle> triangles, const ezColor& color, bool bTwoSided)
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

  if (bTwoSided)
  {
    for (auto& triangle : triangles)
    {
      const ezColorLinearUB col = triangle.m_color * color;

      auto& v1 = data.m_triangleVertices.ExpandAndGetRef();
      auto& v2 = data.m_triangleVertices.ExpandAndGetRef();
      auto& v3 = data.m_triangleVertices.ExpandAndGetRef();

      v1.m_position = triangle.m_position[0];
      v1.m_color = col;
      v2.m_position = triangle.m_position[2];
      v2.m_color = col;
      v3.m_position = triangle.m_position[1];
      v3.m_color = col;
    }
  }
}

void ezDebugRenderer::DrawTexturedTriangles(const ezDebugRendererContext& context, ezArrayPtr<ezDebugRendererTexturedTriangle> triangles, const ezColor& color, const ezTexture2DResourceHandle& hTexture, bool bTwoSided)
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

  if (bTwoSided)
  {
    for (auto& triangle : triangles)
    {
      const ezColorLinearUB col = triangle.m_color * color;

      auto& v1 = data.ExpandAndGetRef();
      auto& v2 = data.ExpandAndGetRef();
      auto& v3 = data.ExpandAndGetRef();

      v1.m_position = triangle.m_position[0];
      v1.m_texCoord = triangle.m_texcoord[0];
      v1.m_color = col;
      v2.m_position = triangle.m_position[2];
      v2.m_texCoord = triangle.m_texcoord[2];
      v2.m_color = col;
      v3.m_position = triangle.m_position[1];
      v3.m_texCoord = triangle.m_texcoord[1];
      v3.m_color = col;
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

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, ezGALTextureResourceViewHandle hResourceView, ezVec2 vScale)
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

void ezDebugRenderer::Draw2DLineRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color)
{
  ezDebugRendererLine lines[4];

  lines[0].m_start = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  lines[0].m_end = ezVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);

  lines[1].m_start = lines[0].m_end;
  lines[1].m_end = ezVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  lines[2].m_start = lines[1].m_end;
  lines[2].m_end = ezVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);

  lines[3].m_start = lines[2].m_end;
  lines[3].m_end = ezVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);

  Draw2DLines(context, lines, color);
}

ezUInt32 ezDebugRenderer::Draw2DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec2I32& vPositionInPixel, const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/, ezDebugTextHAlign::Enum horizontalAlignment /*= ezDebugTextHAlign::Left*/, ezDebugTextVAlign::Enum verticalAlignment /*= ezDebugTextVAlign::Top*/)
{
  return AddTextLines(context, text, vPositionInPixel, uiSizeInPixel, horizontalAlignment, verticalAlignment, [=](PerContextData& ref_data, ezStringView sLine, ezVec2 vTopLeftCorner)
    {
    auto& textLine = ref_data.m_textLines2D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel; });
}


void ezDebugRenderer::DrawInfoText(const ezDebugRendererContext& context, ezDebugTextPlacement::Enum placement, ezStringView sGroupName, const ezFormatString& text, const ezColor& color)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  ezStringBuilder tmp;

  auto& e = data.m_infoTextData[(int)placement].ExpandAndGetRef();
  e.m_group = sGroupName;
  e.m_text = text.GetText(tmp);
  e.m_color = color;
}

ezUInt32 ezDebugRenderer::Draw3DText(const ezDebugRendererContext& context, const ezFormatString& text, const ezVec3& vGlobalPosition, const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/, ezDebugTextHAlign::Enum horizontalAlignment /*= ezDebugTextHAlign::Center*/, ezDebugTextVAlign::Enum verticalAlignment /*= ezDebugTextVAlign::Bottom*/)
{
  return AddTextLines(context, text, ezVec2I32(0), uiSizeInPixel, horizontalAlignment, verticalAlignment, [&](PerContextData& ref_data, ezStringView sLine, ezVec2 vTopLeftCorner)
    {
    auto& textLine = ref_data.m_textLines3D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel;
    textLine.m_position = vGlobalPosition; });
}

void ezDebugRenderer::AddPersistentCross(const ezDebugRendererContext& context, float fSize, const ezColor& color, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Crosses.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_fSize = fSize;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::AddPersistentLineSphere(const ezDebugRendererContext& context, float fRadius, const ezColor& color, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Spheres.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_fRadius = fRadius;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::AddPersistentLineBox(const ezDebugRendererContext& context, const ezVec3& vHalfSize, const ezColor& color, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Boxes.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_vHalfSize = vHalfSize;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::AddPersistentLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Lines.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_Lines = lines;
  item.m_Timeout = data.m_Now + duration;
}


void ezDebugRenderer::AddPersistentInfoText(const ezDebugRendererContext& context, ezDebugTextPlacement::Enum placement, const ezFormatString& text, ezTime duration, const ezColor& color /*= ezColor::White*/)
{
  EZ_LOCK(s_Mutex);

  ezStringBuilder tmp;

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_InfoText.ExpandAndGetRef();
  item.m_sText = text.GetText(tmp);
  item.m_Placement = placement;
  item.m_Color = color;
  item.m_Timeout = data.m_Now + duration;
}

void ezDebugRenderer::DrawAngle(const ezDebugRendererContext& context, ezAngle startAngle, ezAngle endAngle, const ezColor& solidColor, const ezColor& lineColor, ezMatOrTransform mTransform0, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/, ezVec3 vRotationAxis /*= ezVec3::MakeAxisZ()*/)
{
  const ezMat4& transform = mTransform0.m_Mat4;

  ezHybridArray<ezDebugRendererTriangle, 64> tris;
  ezHybridArray<ezDebugRendererLine, 64> lines;

  startAngle.NormalizeRange();
  endAngle.NormalizeRange();

  if (startAngle > endAngle)
    startAngle -= ezAngle::MakeFromDegree(360);

  const ezAngle range = endAngle - startAngle;
  const ezUInt32 uiTesselation = ezMath::Max(1u, (ezUInt32)(range / ezAngle::MakeFromDegree(5)));
  const ezAngle step = range / (float)uiTesselation;

  ezQuat qStart = ezQuat::MakeFromAxisAndAngle(vRotationAxis, startAngle);

  ezQuat qStep = ezQuat::MakeFromAxisAndAngle(vRotationAxis, step);

  ezVec3 vCurDir = qStart * vForwardAxis;

  if (lineColor.a > 0)
  {
    ezDebugRendererLine& l1 = lines.ExpandAndGetRef();
    l1.m_start.SetZero();
    l1.m_end = vCurDir;
  }

  for (ezUInt32 i = 0; i < uiTesselation; ++i)
  {
    const ezVec3 vNextDir = qStep * vCurDir;

    if (solidColor.a > 0)
    {
      ezDebugRendererTriangle& tri1 = tris.ExpandAndGetRef();
      tri1.m_position[0] = transform.GetTranslationVector();
      tri1.m_position[1] = transform.TransformPosition(vNextDir);
      tri1.m_position[2] = transform.TransformPosition(vCurDir);

      ezDebugRendererTriangle& tri2 = tris.ExpandAndGetRef();
      tri2.m_position[0] = transform.GetTranslationVector();
      tri2.m_position[1] = transform.TransformPosition(vCurDir);
      tri2.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (lineColor.a > 0)
    {
      ezDebugRendererLine& l1 = lines.ExpandAndGetRef();
      l1.m_start.SetZero();
      l1.m_end = vNextDir;

      ezDebugRendererLine& l2 = lines.ExpandAndGetRef();
      l2.m_start = vCurDir;
      l2.m_end = vNextDir;
    }

    vCurDir = vNextDir;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void ezDebugRenderer::DrawOpeningCone(const ezDebugRendererContext& context, ezAngle halfAngle, const ezColor& colorInside, const ezColor& colorOutside, ezMatOrTransform mTransform0, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/)
{
  const ezMat4& transform = mTransform0.m_Mat4;

  ezHybridArray<ezDebugRendererTriangle, 64> trisInside;
  ezHybridArray<ezDebugRendererTriangle, 64> trisOutside;

  halfAngle = ezMath::Clamp(halfAngle, ezAngle(), ezAngle::MakeFromDegree(180));

  const ezAngle refAngle = halfAngle <= ezAngle::MakeFromDegree(90) ? halfAngle : ezAngle::MakeFromDegree(180) - halfAngle;
  const ezUInt32 uiTesselation = ezMath::Max(8u, (ezUInt32)(refAngle / ezAngle::MakeFromDegree(2)));

  const ezVec3 tangentAxis = vForwardAxis.GetOrthogonalVector().GetNormalized();

  ezQuat tilt = ezQuat::MakeFromAxisAndAngle(tangentAxis, halfAngle);

  ezQuat step = ezQuat::MakeFromAxisAndAngle(vForwardAxis, ezAngle::MakeFromDegree(360) / (float)uiTesselation);

  ezVec3 vCurDir = tilt * vForwardAxis;

  for (ezUInt32 i = 0; i < uiTesselation; ++i)
  {
    const ezVec3 vNextDir = step * vCurDir;

    if (colorInside.a > 0)
    {
      ezDebugRendererTriangle& tri = trisInside.ExpandAndGetRef();
      tri.m_position[0] = transform.GetTranslationVector();
      tri.m_position[1] = transform.TransformPosition(vCurDir);
      tri.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (colorOutside.a > 0)
    {
      ezDebugRendererTriangle& tri = trisOutside.ExpandAndGetRef();
      tri.m_position[0] = transform.GetTranslationVector();
      tri.m_position[1] = transform.TransformPosition(vNextDir);
      tri.m_position[2] = transform.TransformPosition(vCurDir);
    }

    vCurDir = vNextDir;
  }


  DrawSolidTriangles(context, trisInside, colorInside);
  DrawSolidTriangles(context, trisOutside, colorOutside);
}

void ezDebugRenderer::DrawLimitCone(const ezDebugRendererContext& context, ezAngle halfAngle1, ezAngle halfAngle2, const ezColor& solidColor, const ezColor& lineColor, ezMatOrTransform mTransform0)
{
  const ezMat4& transform = mTransform0.m_Mat4;

  constexpr ezUInt32 NUM_LINES = 32;
  ezHybridArray<ezDebugRendererLine, NUM_LINES * 2> lines;
  ezHybridArray<ezDebugRendererTriangle, NUM_LINES * 2> tris;

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
        t1.m_position[0] = transform.GetTranslationVector();
        t1.m_position[1] = transform.TransformPosition(prev);
        t1.m_position[2] = transform.TransformPosition(a);

        auto& t2 = tris.ExpandAndGetRef();
        t2.m_position[0] = transform.GetTranslationVector();
        t2.m_position[1] = transform.TransformPosition(a);
        t2.m_position[2] = transform.TransformPosition(prev);
      }

      prev = a;
    }
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void ezDebugRenderer::DrawCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, ezMatOrTransform mTransform0, bool bCapStart /*= false*/, bool bCapEnd /*= false*/, ezBasisAxis::Enum cylinderAxis /*= ezBasisAxis::PositiveX*/)
{
  const ezQuat tilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, cylinderAxis);
  const ezMat4 transform = mTransform0.m_Mat4 * tilt.GetAsMat4();

  constexpr ezUInt32 NUM_SEGMENTS = 16;
  ezHybridArray<ezDebugRendererLine, NUM_SEGMENTS * 3> lines;
  ezHybridArray<ezDebugRendererTriangle, NUM_SEGMENTS * 2 * 2> tris;

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

void ezDebugRenderer::DrawArrow(const ezDebugRendererContext& context, float fSize, const ezColor& color, ezMatOrTransform mTransform, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/)
{
  vForwardAxis.Normalize();
  const ezVec3 right = vForwardAxis.GetOrthogonalVector().GetNormalized();
  const ezVec3 up = vForwardAxis.CrossRH(right).GetNormalized();
  const ezVec3 endPoint = vForwardAxis * fSize;
  const ezVec3 endPoint2 = vForwardAxis * fSize * 0.9f;
  const float tipSize = fSize * 0.1f;

  ezDebugRendererLine lines[9];
  lines[0] = ezDebugRendererLine(ezVec3::MakeZero(), endPoint);
  lines[1] = ezDebugRendererLine(endPoint, endPoint2 + right * tipSize);
  lines[2] = ezDebugRendererLine(endPoint, endPoint2 + up * tipSize);
  lines[3] = ezDebugRendererLine(endPoint, endPoint2 - right * tipSize);
  lines[4] = ezDebugRendererLine(endPoint, endPoint2 - up * tipSize);
  lines[5] = ezDebugRendererLine(lines[1].m_end, lines[2].m_end);
  lines[6] = ezDebugRendererLine(lines[2].m_end, lines[3].m_end);
  lines[7] = ezDebugRendererLine(lines[3].m_end, lines[4].m_end);
  lines[8] = ezDebugRendererLine(lines[4].m_end, lines[1].m_end);

  DrawLines(context, lines, color, mTransform);
}

// static
float ezDebugRenderer::GetTextGlyphWidth(ezUInt32 uiSizeInPixel /*= 16*/)
{
  // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
  return ezMath::Ceil(uiSizeInPixel * cvar_DebugTextScale * (8.0f / 16.0f));
}

// static
float ezDebugRenderer::GetTextLineHeight(ezUInt32 uiSizeInPixel /*= 16*/)
{
  return ezMath::Ceil(uiSizeInPixel * cvar_DebugTextScale * (20.0f / 16.0f));
}

// static
float ezDebugRenderer::GetTextScale()
{
  return cvar_DebugTextScale;
}

// static
void ezDebugRenderer::SetTextScale(float fScale)
{
  cvar_DebugTextScale = fScale;
}

// static
void ezDebugRenderer::RenderWorldSpace(const ezRenderViewContext& renderViewContext)
{
  EZ_PROFILE_SCOPE("ezDebugRenderer::RenderWorldSpace");

  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

// static
void ezDebugRenderer::RenderInternalWorldSpace(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext)
{
  {
    EZ_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = ezClock::GetGlobalClock()->GetLastUpdateTime();

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
          ezDebugRenderer::DrawCross(context, ezVec3::MakeZero(), item.m_fSize, item.m_Color, item.m_Transform);

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
          ezDebugRenderer::DrawLineSphere(context, ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), item.m_fRadius), item.m_Color, item.m_Transform);

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
          ezDebugRenderer::DrawLineBox(context, ezBoundingBox::MakeFromMinMax(-item.m_vHalfSize, item.m_vHalfSize), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent lines
    {
      ezUInt32 uiNumItems = data.m_Lines.GetCount();
      for (ezUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Lines[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Lines.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          ezDebugRenderer::DrawLines(context, item.m_Lines.GetArrayPtr(), item.m_Color, item.m_Transform);

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

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

  // SolidBoxes
  {
    ezUInt32 uiNumSolidBoxes = pData->m_solidBoxes.GetCount();
    if (uiNumSolidBoxes != 0)
    {
      CreateDataBuffer(BufferType::SolidBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);

      const BoxData* pSolidBoxData = pData->m_solidBoxes.GetData();
      while (uiNumSolidBoxes > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::SolidBoxes].GetNewBuffer();
        renderViewContext.m_pRenderContext->BindBuffer("boxData", pDevice->GetDefaultResourceView(hBuffer));
        const ezUInt32 uiNumSolidBoxesInBatch = ezMath::Min<ezUInt32>(uiNumSolidBoxes, BOXES_PER_BATCH);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

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
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Triangles3D].GetNewBuffer();

        const ezUInt32 uiNumTriangleVerticesInBatch = ezMath::Min<ezUInt32>(uiNumTriangleVertices, TRIANGLE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumTriangleVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(hBuffer, ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNumTriangleVerticesInBatch / 3);

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
      auto hTextureView = itTex.Key();
      const auto format = pDevice->GetResourceView(hTextureView)->GetResource()->GetDescription().m_Format;
      const bool bMonochrome = ezGALResourceFormat::GetChannelCount(format) == 1;

      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", hTextureView);

      const auto& verts = itTex.Value();

      ezUInt32 uiNumVertices = verts.GetCount();
      if (uiNumVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles3D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MONOCHROME", bMonochrome ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNumVertices > 0)
        {
          ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::TexTriangles3D].GetNewBuffer();
          const ezUInt32 uiNumVerticesInBatch = ezMath::Min<ezUInt32>(uiNumVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          EZ_ASSERT_DEV(uiNumVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pTriangleData, uiNumVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

          renderViewContext.m_pRenderContext->BindMeshBuffer(hBuffer, ezGALBufferHandle(), &s_TexVertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNumVerticesInBatch / 3);

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
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Lines].GetNewBuffer();
        const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(hBuffer, ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

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

      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);

      const BoxData* pLineBoxData = pData->m_lineBoxes.GetData();
      while (uiNumLineBoxes > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::LineBoxes].GetNewBuffer();
        const ezUInt32 uiNumLineBoxesInBatch = ezMath::Min<ezUInt32>(uiNumLineBoxes, BOXES_PER_BATCH);
        renderViewContext.m_pRenderContext->BindBuffer("boxData", pDevice->GetDefaultResourceView(hBuffer));
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch).IgnoreResult();

        uiNumLineBoxes -= uiNumLineBoxesInBatch;
        pLineBoxData += BOXES_PER_BATCH;
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
        renderViewContext.m_pViewData->ConvertScreenNormalizedPosToPixelPos(screenPos);

        textLine.m_topLeftCorner.x += ezMath::Round(screenPos.x);
        textLine.m_topLeftCorner.y += ezMath::Round(screenPos.y);

        AppendGlyphs(pData->m_glyphs, textLine);
      }
    }

    ezUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);

      renderViewContext.m_pRenderContext->BindTexture2D("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Glyphs].GetNewBuffer();
        const ezUInt32 uiNumGlyphsInBatch = ezMath::Min<ezUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        renderViewContext.m_pRenderContext->BindBuffer("glyphData", pDevice->GetDefaultResourceView(hBuffer));
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

// static
void ezDebugRenderer::RenderScreenSpace(const ezRenderViewContext& renderViewContext)
{
  EZ_PROFILE_SCOPE("ezDebugRenderer::RenderScreenSpace");

  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

// static
void ezDebugRenderer::RenderInternalScreenSpace(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext)
{
  {
    EZ_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = ezClock::GetGlobalClock()->GetLastUpdateTime();

    // persistent info text
    {
      ezUInt32 uiNumItems = data.m_InfoText.GetCount();
      for (ezUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_InfoText[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_InfoText.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          ezDebugRenderer::DrawInfoText(context, item.m_Placement, "__Persistent", item.m_sText.GetView(), item.m_Color);

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
    static_assert((int)ezDebugTextPlacement::ENUM_COUNT == 6);

    ezDebugTextHAlign::Enum ha[(int)ezDebugTextPlacement::ENUM_COUNT] = {
      ezDebugTextHAlign::Left,
      ezDebugTextHAlign::Center,
      ezDebugTextHAlign::Right,
      ezDebugTextHAlign::Left,
      ezDebugTextHAlign::Center,
      ezDebugTextHAlign::Right};

    ezDebugTextVAlign::Enum va[(int)ezDebugTextPlacement::ENUM_COUNT] = {
      ezDebugTextVAlign::Top,
      ezDebugTextVAlign::Top,
      ezDebugTextVAlign::Top,
      ezDebugTextVAlign::Bottom,
      ezDebugTextVAlign::Bottom,
      ezDebugTextVAlign::Bottom};

    int lineHeight = (int)GetTextLineHeight();

    ezInt32 resX = (ezInt32)renderViewContext.m_pViewData->m_ViewPortRect.width;
    ezInt32 resY = (ezInt32)renderViewContext.m_pViewData->m_ViewPortRect.height;

    ezVec2I32 anchor[(int)ezDebugTextPlacement::ENUM_COUNT] = {
      ezVec2I32(10, 10),
      ezVec2I32(resX / 2, 10),
      ezVec2I32(resX - 10, 10),
      ezVec2I32(10, resY - 10),
      ezVec2I32(resX / 2, resY - 10),
      ezVec2I32(resX - 10, resY - 10)};

    for (ezUInt32 corner = 0; corner < (ezUInt32)ezDebugTextPlacement::ENUM_COUNT; ++corner)
    {
      auto& cd = pData->m_infoTextData[corner];

      // InsertionSort is stable
      ezSorting::InsertionSort(cd, [](const InfoTextData& lhs, const InfoTextData& rhs) -> bool
        { return lhs.m_group < rhs.m_group; });

      ezVec2I32 pos = anchor[corner];
      int offset = va[corner] == ezDebugTextVAlign::Top ? lineHeight : -lineHeight;

      for (ezUInt32 i = 0; i < cd.GetCount(); ++i)
      {
        // add some space between groups
        if (i > 0 && cd[i - 1].m_group != cd[i].m_group)
          pos.y += offset;

        pos.y += offset * Draw2DText(context, cd[i].m_text.GetData(), pos, cd[i].m_color, 16, ha[corner], va[corner]);
      }
    }
  }

  // update the frame counter
  pDoubleBufferedContextData->m_uiLastRenderedFrame = ezRenderWorld::GetFrameCounter();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

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
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Triangles2D].GetNewBuffer();
        const ezUInt32 uiNum2DVerticesInBatch = ezMath::Min<ezUInt32>(uiNum2DVertices, TRIANGLE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(hBuffer, ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

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
      auto hTextureView = itTex.Key();
      const auto format = pDevice->GetResourceView(hTextureView)->GetResource()->GetDescription().m_Format;
      const bool bMonochrome = ezGALResourceFormat::GetChannelCount(format) == 1;

      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", hTextureView);

      const auto& verts = itTex.Value();

      ezUInt32 uiNum2DVertices = verts.GetCount();
      if (uiNum2DVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles2D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MONOCHROME", bMonochrome ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNum2DVertices > 0)
        {
          ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::TexTriangles2D].GetNewBuffer();
          const ezUInt32 uiNum2DVerticesInBatch = ezMath::Min<ezUInt32>(uiNum2DVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          EZ_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

          renderViewContext.m_pRenderContext->BindMeshBuffer(hBuffer, ezGALBufferHandle(), &s_TexVertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNum2DVertices -= uiNum2DVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
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
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Lines2D].GetNewBuffer();
        const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(hBuffer, ezGALBufferHandle(), &s_VertexDeclarationInfo, ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // Text
  {
    pData->m_glyphs.Clear();

    for (auto& textLine : pData->m_textLines2D)
    {
      AppendGlyphs(pData->m_glyphs, textLine);
    }

    ezUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);
      renderViewContext.m_pRenderContext->BindTexture2D("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Glyphs].GetNewBuffer();
        const ezUInt32 uiNumGlyphsInBatch = ezMath::Min<ezUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        renderViewContext.m_pRenderContext->BindBuffer("glyphData", pDevice->GetDefaultResourceView(hBuffer));
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

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
    // BEGIN-DOCS-CODE-SNIPPET: resource-management-create
    ezGeometry geom;
    geom.AddBox(ezVec3(2.0f), false);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
    // END-DOCS-CODE-SNIPPET
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
    memoryDesc.m_pData = debugFontImage.GetByteBlobPtr();
    memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(debugFontImage.GetDepthPitch());

    ezTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = debugFontImage.GetWidth();
    desc.m_DescGAL.m_uiHeight = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RUByteNormalized;
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

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Debug, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetResolution),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(0.1f)),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(ezVec3(1))),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawSolidBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(ezVec3(1))),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute())),
    
    EZ_SCRIPT_FUNCTION_PROPERTY(Draw2DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel", In, "HAlign")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(16))),

    EZ_SCRIPT_FUNCTION_PROPERTY(Draw3DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(16))),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawInfoText, In, "World", In, "Text", In, "Placement", In, "Group", In, "Color"),

    EZ_SCRIPT_FUNCTION_PROPERTY(AddPersistentCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(0.1f)),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1)))),

    EZ_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(ezVec3(1))),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1)))),

    EZ_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1)))),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(4, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(Draw2DLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(4, new ezExposeColorAlphaAttribute())),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Debug"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezVec2 ezScriptExtensionClass_Debug::GetResolution()
{
  for (const ezViewHandle& hView : ezRenderWorld::GetMainViews())
  {
    ezView* pView;
    if (ezRenderWorld::TryGetView(hView, pView))
    {
      return ezVec2(pView->GetViewport().width, pView->GetViewport().height);
    }
  }

  return ezVec2::MakeZero();
}

// static
void ezScriptExtensionClass_Debug::DrawCross(const ezWorld* pWorld, const ezVec3& vPosition, float fSize, const ezColor& color, const ezTransform& transform)
{
  ezDebugRenderer::DrawCross(pWorld, vPosition, fSize, color, transform);
}

// static
void ezScriptExtensionClass_Debug::DrawLineBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform)
{
  ezDebugRenderer::DrawLineBox(pWorld, ezBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void ezScriptExtensionClass_Debug::DrawLineSphere(const ezWorld* pWorld, const ezVec3& vPosition, float fRadius, const ezColor& color, const ezTransform& transform)
{
  ezDebugRenderer::DrawLineSphere(pWorld, ezBoundingSphere::MakeFromCenterAndRadius(vPosition, fRadius), color, transform);
}

// static
void ezScriptExtensionClass_Debug::DrawSolidBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform)
{
  ezDebugRenderer::DrawSolidBox(pWorld, ezBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void ezScriptExtensionClass_Debug::Draw2DText(const ezWorld* pWorld, ezStringView sText, const ezVec3& vPosition, const ezColor& color, ezUInt32 uiSizeInPixel, ezEnum<ezDebugTextHAlign> horizontalAlignment)
{
  ezVec2I32 vPositionInPixel = ezVec2I32(static_cast<int>(ezMath::Round(vPosition.x)), static_cast<int>(ezMath::Round(vPosition.y)));
  ezDebugRenderer::Draw2DText(pWorld, sText, vPositionInPixel, color, uiSizeInPixel, horizontalAlignment);
}

// static
void ezScriptExtensionClass_Debug::Draw3DText(const ezWorld* pWorld, ezStringView sText, const ezVec3& vPosition, const ezColor& color, ezUInt32 uiSizeInPixel)
{
  ezDebugRenderer::Draw3DText(pWorld, sText, vPosition, color, uiSizeInPixel);
}

// static
void ezScriptExtensionClass_Debug::DrawInfoText(const ezWorld* pWorld, ezStringView sText, ezEnum<ezDebugTextPlacement> placement, ezStringView sGroupName, const ezColor& color)
{
  ezDebugRenderer::DrawInfoText(pWorld, placement, sGroupName, sText, color);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentCross(const ezWorld* pWorld, const ezVec3& vPosition, float fSize, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  ezTransform t = transform;
  t.m_vPosition += vPosition;

  ezDebugRenderer::AddPersistentCross(pWorld, fSize, color, t, duration);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentLineBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  ezTransform t = transform;
  t.m_vPosition += vPosition;

  ezDebugRenderer::AddPersistentLineBox(pWorld, vHalfExtents, color, t, duration);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentLineSphere(const ezWorld* pWorld, const ezVec3& vPosition, float fRadius, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  ezTransform t = transform;
  t.m_vPosition += vPosition;

  ezDebugRenderer::AddPersistentLineSphere(pWorld, fRadius, color, t, duration);
}

void ezScriptExtensionClass_Debug::DrawLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor)
{
  ezDebugRendererLine line[1];
  line[0].m_start = vStart;
  line[0].m_end = vEnd;
  line[0].m_startColor = startColor;
  line[0].m_endColor = endColor;

  ezDebugRenderer::DrawLines(pWorld, line, ezColor::White);
}

void ezScriptExtensionClass_Debug::Draw2DLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor)
{
  ezDebugRendererLine line[1];
  line[0].m_start = vStart;
  line[0].m_end = vEnd;
  line[0].m_startColor = startColor;
  line[0].m_endColor = endColor;

  ezDebugRenderer::Draw2DLines(pWorld, line, ezColor::White);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugRenderer);
