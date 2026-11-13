#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugShapes.h>
#include <RendererCore/Debug/SimpleASCIIFont.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/BufferPool.h>
#include <RendererFoundation/Shader/Types.h>

ezCVarFloat cvar_AppTextScale("App.TextScale", 1.0f, ezCVarFlags::Save, "Global scale for debug text");
ezCVarBool cvar_DebugRenderOccluded("Debug.RenderOccluded", true, ezCVarFlags::Save, "Also render debug geometry behind opaque surfaces, dimmed.");

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
  struct Vertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
  };

  static_assert(sizeof(Vertex) == 16);

  struct TexVertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
    ezVec2 m_texCoord;
  };

  static_assert(sizeof(TexVertex) == 24);

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

  struct alignas(16) ThickLineData
  {
    ezVec3 m_start;
    float m_fThickness;
    ezVec3 m_end;
    ezColorLinearUB m_startColor;
    ezColorLinearUB m_endColor;
    ezUInt32 m_padding[3];
  };

  static_assert(sizeof(ThickLineData) == 48);

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
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_lineOccludedVertices;
    ezDynamicArray<ThickLineData, ezAlignedAllocatorWrapper> m_thickLines;
    ezDynamicArray<ThickLineData, ezAlignedAllocatorWrapper> m_thickLinesOccluded;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangleVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangle2DVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_line2DVertices;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_lineBoxes;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_solidBoxes;
    ezMap<ezGALTextureHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle2DVertices;
    ezMap<ezGALTextureHandle, ezDynamicArray<TexVertex, ezAlignedAllocatorWrapper>> m_texTriangle3DVertices;

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
        pData->m_lineOccludedVertices.Clear();
        pData->m_thickLines.Clear();
        pData->m_thickLinesOccluded.Clear();
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
      ThickLines,

      Count
    };
  };

  static ezGALBufferPool s_DataBuffer[BufferType::Count];

  static ezMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  static ezMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  static ezGALVertexAttribute s_VertexAttributes[2];
  static ezGALVertexAttribute s_TexVertexAttributes[3];
  static ezTexture2DResourceHandle s_hDebugFontTexture;

  static ezShaderResourceHandle s_hDebugGeometryShader;
  static ezShaderResourceHandle s_hDebugPrimitiveShader;
  static ezShaderResourceHandle s_hDebugTexturedPrimitiveShader;
  static ezShaderResourceHandle s_hDebugTextShader;
  static ezShaderResourceHandle s_hDebugThickLinesShader;

  enum
  {
    DEBUG_BUFFER_SIZE = 1024 * 256,
    BOXES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
    TEX_TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(TexVertex) / 3) * 3,
    GLYPHS_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(GlyphData),
    THICK_LINES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(ThickLineData),
  };

  static void CreateDataBuffer(BufferType::Enum bufferType, ezUInt32 uiStructSize)
  {
    if (!s_DataBuffer[bufferType].IsInitialized())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiStructSize;
      // A structured buffer must hold a whole number of elements.
      desc.m_uiTotalSize = (DEBUG_BUFFER_SIZE / uiStructSize) * uiStructSize;
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

    ezTempHybridArray<ezStringView, 8> lines;
    ezUInt32 maxLineLength = 0;

    ezTempHybridArray<ezUInt32, 8> maxColumWidth;
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
    const float fGlyphHeight = ezMath::Ceil(uiSizeInPixel * ezDebugRenderer::GetTextScale());
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

  /// fScale scales the glyph size and spacing, but not m_topLeftCorner.
  static void AppendGlyphs(ezDynamicArray<GlyphData, ezAlignedAllocatorWrapper>& ref_glyphs, const TextLineData2D& textLine, float fScale = 1.0f)
  {
    ezVec2 currentPos = textLine.m_topLeftCorner;
    const float fGlyphWidth = ezDebugRenderer::GetTextGlyphWidth(textLine.m_uiSizeInPixel) * fScale;
    const ezUInt16 uiGlyphSize = (ezUInt16)ezMath::Max(1.0f, ezMath::Ceil(textLine.m_uiSizeInPixel * ezDebugRenderer::GetTextScale() * fScale));

    for (ezUInt32 uiCharacter : textLine.m_text)
    {
      auto& glyphData = ref_glyphs.ExpandAndGetRef();
      glyphData.m_topLeftCorner = currentPos;
      glyphData.m_color = textLine.m_color;
      glyphData.m_glyphIndex = uiCharacter < 128 ? static_cast<ezUInt16>(uiCharacter) : 0;
      glyphData.m_sizeInPixel = uiGlyphSize;

      currentPos.x += fGlyphWidth;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Persistent Items

  /// All persistent shapes are stored as their generated lines and triangles, already transformed.
  struct PersistentShapeData
  {
    ezDynamicArray<ezDebugRendererLine> m_Lines;
    ezDynamicArray<ezDebugRendererTriangle> m_Triangles;
    ezColor m_LineColor;
    ezColor m_SolidColor;
    float m_fLineThickness = 0.0f;
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
    ezDeque<PersistentShapeData> m_Shapes;
    ezDeque<PersistentInfoTextData> m_InfoText;
  };

  static ezHashTable<ezDebugRendererContext, PersistentPerContextData> s_PersistentPerContextData;

  /// Adds a new persistent shape, the caller fills in the geometry. Requires s_Mutex to be locked.
  static PersistentShapeData& AddPersistentShape(const ezDebugRendererContext& context, const ezColor& lineColor, const ezColor& solidColor, ezTime duration, float fLineThickness)
  {
    auto& data = s_PersistentPerContextData[context];
    auto& item = data.m_Shapes.ExpandAndGetRef();
    item.m_LineColor = lineColor;
    item.m_SolidColor = solidColor;
    item.m_fLineThickness = fLineThickness;
    item.m_Timeout = data.m_Now + duration;
    return item;
  }

  /// Adds lines to the thin or thick line buffers of the context, depending on the thickness.
  static void AddLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, const ezMat4& mTransform, float fLineThickness, bool bOccluded)
  {
    if (lines.IsEmpty())
      return;

    EZ_LOCK(s_Mutex);

    auto& data = GetDataForExtraction(context);

    if (fLineThickness > 0.0f)
    {
      auto& thickLines = bOccluded ? data.m_thickLinesOccluded : data.m_thickLines;
      thickLines.Reserve(thickLines.GetCount() + lines.GetCount());

      for (auto& line : lines)
      {
        auto& lineData = thickLines.ExpandAndGetRef();
        lineData.m_start = mTransform.TransformPosition(line.m_start);
        lineData.m_end = mTransform.TransformPosition(line.m_end);
        lineData.m_fThickness = fLineThickness;
        lineData.m_startColor = line.m_startColor * color;
        lineData.m_endColor = line.m_endColor * color;
        lineData.m_padding[0] = 0;
        lineData.m_padding[1] = 0;
        lineData.m_padding[2] = 0;
      }
    }
    else
    {
      auto& vertices = bOccluded ? data.m_lineOccludedVertices : data.m_lineVertices;
      vertices.Reserve(vertices.GetCount() + lines.GetCount() * 2);

      for (auto& line : lines)
      {
        vertices.PushBack({mTransform.TransformPosition(line.m_start), line.m_startColor * color});
        vertices.PushBack({mTransform.TransformPosition(line.m_end), line.m_endColor * color});
      }
    }
  }

} // namespace

static void OnEngineStartup();
static void OnEngineShutdown();
static void RenderInternalWorldSpace(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext);
static void RenderInternalScreenSpace(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DebugRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void ezDebugRenderer::DrawLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  AddLines(context, lines, color, mTransform.m_Mat4, fLineThickness, false);
}

void ezDebugRenderer::DrawLinesOccluded(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  AddLines(context, lines, color, mTransform.m_Mat4, fLineThickness, true);
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
    const ezColor* pColors = &line.m_startColor;

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_line2DVertices.ExpandAndGetRef();
      vertex.m_position = pPositions[i];
      vertex.m_color = pColors[i] * color;
    }
  }
}

void ezDebugRenderer::DrawCross(const ezDebugRendererContext& context, const ezVec3& vGlobalPosition, float fLineLength, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezHybridArray<ezDebugRendererLine, 3> lines;
  ezDebugShapes::AppendLineCross(lines, vGlobalPosition, fLineLength, mTransform);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 3, "Incorrect in-place buffer size");
}

void ezDebugRenderer::DrawLineBox(const ezDebugRendererContext& context, const ezBoundingBox& box, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  if (fLineThickness > 0.0f)
  {
    ezHybridArray<ezDebugRendererLine, 12> lines;
    ezDebugShapes::AppendLineBox(lines, box, mTransform);
    DrawLines(context, lines, color, fLineThickness);
    EZ_ASSERT_DEBUG(lines.GetCapacity() == 12, "Incorrect in-place buffer size");
    return;
  }

  // thin line boxes are rendered as instanced meshes
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();

  ezTransform boxTransform(box.GetCenter(), ezQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = mTransform.m_Mat4 * boxTransform.GetAsMat4();
  boxData.m_color = color;
}

void ezDebugRenderer::DrawLineBoxCorners(const ezDebugRendererContext& context, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezHybridArray<ezDebugRendererLine, 24> lines;
  ezDebugShapes::AppendLineBoxCorners(lines, box, fCornerFraction, mTransform);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 24, "Incorrect in-place buffer size");
}

void ezDebugRenderer::DrawLineSphere(const ezDebugRendererContext& context, const ezBoundingSphere& sphere, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezHybridArray<ezDebugRendererLine, 96> lines;
  ezDebugShapes::AppendLineSphere(lines, sphere, mTransform);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 96, "Incorrect in-place buffer size");
}

void ezDebugRenderer::DrawLineCapsuleZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezHybridArray<ezDebugRendererLine, 132> lines;
  ezDebugShapes::AppendLineCapsuleZ(lines, fLength, fRadius, mTransform);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 132, "Incorrect in-place buffer size");
}

void ezDebugRenderer::DrawLineCylinderZ(const ezDebugRendererContext& context, float fLength, float fRadius, const ezColor& color, float fLineThickness /*= 0.0f*/, ezMatOrTransform mTransform /*= ezMat4::MakeIdentity()*/)
{
  ezHybridArray<ezDebugRendererLine, 68> lines;
  ezDebugShapes::AppendLineCylinderZ(lines, fLength, fRadius, mTransform);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 68, "Incorrect in-place buffer size");
}

void ezDebugRenderer::DrawLineFrustum(const ezDebugRendererContext& context, const ezFrustum& frustum, const ezColor& color, float fLineThickness /*= 0.0f*/, bool bDrawPlaneNormals /*= false*/)
{
  ezVec3 cornerPoints[8];
  if (frustum.ComputeCornerPoints(cornerPoints).Failed())
    return;

  ezHybridArray<ezDebugRendererLine, 12> lines;
  ezDebugShapes::AppendLineFrustum(lines, frustum);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 12, "Incorrect in-place buffer size");

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

    DrawLines(context, normalLines, normalColor, fLineThickness);
  }
}

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

void ezDebugRenderer::DrawSolidTriangles(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererTriangle> triangles, const ezColor& color, bool bTwoSided)
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
  auto hGalTexture = pTexture->GetGALTexture();

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context).m_texTriangle3DVertices[hGalTexture];

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
  Draw2DRectangle(context, rectInPixel, fDepth, color, pTexture->GetGALTexture(), vScale);
}

void ezDebugRenderer::Draw2DRectangle(const ezDebugRendererContext& context, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color, ezGALTextureHandle hResourceView, ezVec2 vScale)
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

void ezDebugRenderer::Draw2DTriangles(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererTriangle> triangles, const ezColor& color)
{
  if (triangles.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& triangle : triangles)
  {
    // the 2D triangles are rendered with back-face culling, use the same winding as Draw2DRectangle()
    const ezVec3 e1 = triangle.m_position[1] - triangle.m_position[0];
    const ezVec3 e2 = triangle.m_position[2] - triangle.m_position[0];
    const bool bFlip = (e1.x * e2.y - e1.y * e2.x) < 0.0f;

    for (ezUInt32 i : {0u, bFlip ? 2u : 1u, bFlip ? 1u : 2u})
    {
      auto& vertex = data.m_triangle2DVertices.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_color = triangle.m_color * color;
    }
  }
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

void ezDebugRenderer::AddPersistentCross(const ezDebugRendererContext& context, float fSize, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& item = AddPersistentShape(context, color, ezColor::MakeZero(), duration, fLineThickness);
  ezDebugShapes::AppendLineCross(item.m_Lines, ezVec3::MakeZero(), fSize, mTransform);
}

void ezDebugRenderer::AddPersistentLineSphere(const ezDebugRendererContext& context, float fRadius, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& item = AddPersistentShape(context, color, ezColor::MakeZero(), duration, fLineThickness);
  ezDebugShapes::AppendLineSphere(item.m_Lines, ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), fRadius), mTransform);
}

void ezDebugRenderer::AddPersistentLineBox(const ezDebugRendererContext& context, const ezVec3& vHalfSize, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& item = AddPersistentShape(context, color, ezColor::MakeZero(), duration, fLineThickness);
  ezDebugShapes::AppendLineBox(item.m_Lines, ezBoundingBox::MakeFromMinMax(-vHalfSize, vHalfSize), mTransform);
}

void ezDebugRenderer::AddPersistentLines(const ezDebugRendererContext& context, ezArrayPtr<const ezDebugRendererLine> lines, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezTime duration)
{
  EZ_LOCK(s_Mutex);

  auto& item = AddPersistentShape(context, color, ezColor::MakeZero(), duration, fLineThickness);
  item.m_Lines.Reserve(lines.GetCount());

  for (const auto& line : lines)
  {
    auto& l = item.m_Lines.ExpandAndGetRef();
    l = line;
    l.m_start = mTransform.m_Mat4.TransformPosition(line.m_start);
    l.m_end = mTransform.m_Mat4.TransformPosition(line.m_end);
  }
}

void ezDebugRenderer::AddPersistentCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform, ezTime duration, bool bCapStart /*= false*/, bool bCapEnd /*= false*/, ezBasisAxis::Enum cylinderAxis /*= ezBasisAxis::PositiveX*/)
{
  EZ_LOCK(s_Mutex);

  auto& item = AddPersistentShape(context, lineColor, solidColor, duration, fLineThickness);
  if (lineColor.a > 0)
    ezDebugShapes::AppendLineCylinder(item.m_Lines, fRadiusStart, fRadiusEnd, fLength, mTransform, cylinderAxis);

  if (solidColor.a > 0)
    ezDebugShapes::AppendSolidCylinder(item.m_Triangles, fRadiusStart, fRadiusEnd, fLength, mTransform, bCapStart, bCapEnd, cylinderAxis);
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

void ezDebugRenderer::DrawAngle(const ezDebugRendererContext& context, ezAngle startAngle, ezAngle endAngle, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/, ezVec3 vRotationAxis /*= ezVec3::MakeAxisZ()*/)
{
  ezTempHybridArray<ezDebugRendererTriangle, 64> tris;
  ezTempHybridArray<ezDebugRendererLine, 64> lines;

  if (lineColor.a > 0)
    ezDebugShapes::AppendLineAngle(lines, startAngle, endAngle, mTransform, vForwardAxis, vRotationAxis);

  if (solidColor.a > 0)
    ezDebugShapes::AppendSolidAngle(tris, startAngle, endAngle, mTransform, vForwardAxis, vRotationAxis);

  DrawSolidTriangles(context, tris, solidColor, true);
  DrawLines(context, lines, lineColor, fLineThickness);
}

void ezDebugRenderer::DrawOpeningCone(const ezDebugRendererContext& context, ezAngle halfAngle, const ezColor& colorInside, const ezColor& colorOutside, ezMatOrTransform mTransform0, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/)
{
  const ezMat4& transform = mTransform0.m_Mat4;

  ezTempHybridArray<ezDebugRendererTriangle, 64> trisInside;
  ezTempHybridArray<ezDebugRendererTriangle, 64> trisOutside;

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

void ezDebugRenderer::DrawLimitCone(const ezDebugRendererContext& context, ezAngle halfAngle1, ezAngle halfAngle2, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform)
{
  ezTempHybridArray<ezDebugRendererLine, 66> lines;
  ezTempHybridArray<ezDebugRendererTriangle, 66> tris;

  if (lineColor.a > 0)
    ezDebugShapes::AppendLineLimitCone(lines, halfAngle1, halfAngle2, mTransform);

  if (solidColor.a > 0)
    ezDebugShapes::AppendSolidLimitCone(tris, halfAngle1, halfAngle2, mTransform);

  DrawSolidTriangles(context, tris, solidColor, true);
  DrawLines(context, lines, lineColor, fLineThickness);
}

void ezDebugRenderer::DrawCylinder(const ezDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, float fLineThickness, ezMatOrTransform mTransform, bool bCapStart /*= false*/, bool bCapEnd /*= false*/, ezBasisAxis::Enum cylinderAxis /*= ezBasisAxis::PositiveX*/)
{
  ezTempHybridArray<ezDebugRendererLine, 48> lines;
  ezTempHybridArray<ezDebugRendererTriangle, 64> tris;

  if (lineColor.a > 0)
    ezDebugShapes::AppendLineCylinder(lines, fRadiusStart, fRadiusEnd, fLength, mTransform, cylinderAxis);

  if (solidColor.a > 0)
    ezDebugShapes::AppendSolidCylinder(tris, fRadiusStart, fRadiusEnd, fLength, mTransform, bCapStart, bCapEnd, cylinderAxis);

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, fLineThickness);
}

void ezDebugRenderer::DrawArrow(const ezDebugRendererContext& context, float fSize, const ezColor& color, float fLineThickness, ezMatOrTransform mTransform, ezVec3 vForwardAxis /*= ezVec3::MakeAxisX()*/)
{
  ezHybridArray<ezDebugRendererLine, 9> lines;
  ezDebugShapes::AppendLineArrow(lines, fSize, mTransform, vForwardAxis);
  DrawLines(context, lines, color, fLineThickness);
  EZ_ASSERT_DEBUG(lines.GetCapacity() == 9, "Incorrect in-place buffer size");
}

float ezDebugRenderer::GetTextGlyphWidth(ezUInt32 uiSizeInPixel /*= 16*/)
{
  // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
  return ezMath::Ceil(uiSizeInPixel * GetTextScale() * (8.0f / 16.0f));
}

float ezDebugRenderer::GetTextLineHeight(ezUInt32 uiSizeInPixel /*= 16*/)
{
  return ezMath::Ceil(uiSizeInPixel * GetTextScale() * (20.0f / 16.0f));
}

float ezDebugRenderer::GetTextScale()
{
  return cvar_AppTextScale * ezRenderWorld::GetDisplayScale();
}

void ezDebugRendererInternal::RenderWorldSpace(const ezRenderViewContext& renderViewContext)
{
  EZ_PROFILE_SCOPE("ezDebugRendererInternal::RenderWorldSpace");

  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

/// Renders thick lines with the current OCCLUDED_PASS permutation.
///
/// The same data can be rendered by several views, thus the thickness is scaled by the display scale and the render scale of the view here.
static void RenderThickLines(const ezRenderViewContext& renderViewContext, ezArrayPtr<const ThickLineData> lines)
{
  if (lines.IsEmpty())
    return;

  ezGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();
  ezBindGroupBuilder& bindGroupRenderPass = ezRenderContext::GetDefaultInstance()->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);

  CreateDataBuffer(BufferType::ThickLines, sizeof(ThickLineData));

  renderViewContext.m_pRenderContext->BindShader(s_hDebugThickLinesShader);

  const float fThicknessScale = ezRenderWorld::GetDisplayScale() * renderViewContext.m_pViewData->m_fRenderScale;
  ezDynamicArray<ThickLineData, ezAlignedAllocatorWrapper> scaledLines;

  while (!lines.IsEmpty())
  {
    const ezUInt32 uiNumLinesInBatch = ezMath::Min<ezUInt32>(lines.GetCount(), THICK_LINES_PER_BATCH);
    ezArrayPtr<const ThickLineData> batch = lines.GetSubArray(0, uiNumLinesInBatch);

    if (fThicknessScale != 1.0f)
    {
      scaledLines = batch;

      for (auto& line : scaledLines)
      {
        line.m_fThickness *= fThicknessScale;
      }

      batch = scaledLines;
    }

    ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::ThickLines].GetNewBuffer();
    bindGroupRenderPass.BindBuffer("lineData", hBuffer);
    pGALCommandEncoder->UpdateBuffer(hBuffer, 0, batch.ToByteArray(), ezGALUpdateMode::AheadOfTime);

    // Two triangles per line, the vertex shader expands them from the line data.
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, uiNumLinesInBatch * 2);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    lines = lines.GetSubArray(uiNumLinesInBatch);
  }
}

static void RenderInternalWorldSpace(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext)
{
  {
    EZ_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = ezClock::GetGlobalClock()->GetLastUpdateTime();

    for (ezUInt32 i = 0; i < data.m_Shapes.GetCount();)
    {
      const auto& item = data.m_Shapes[i];

      if (data.m_Now > item.m_Timeout)
      {
        data.m_Shapes.RemoveAtAndSwap(i);
        continue;
      }

      ezDebugRenderer::DrawSolidTriangles(context, item.m_Triangles, item.m_SolidColor);
      ezDebugRenderer::DrawLines(context, item.m_Lines, item.m_LineColor, item.m_fLineThickness);

      ++i;
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

  ezBindGroupBuilder& bindGroupRenderPass = ezRenderContext::GetDefaultInstance()->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);

  // 3D Lines that show through geometry (no depth test, distance fade).
  // Rendered first so the depth-tested regular line buffer below can override the visible portions.
  // Globally disabled via the CVar 'Debug.RenderOccluded'.
  if (cvar_DebugRenderOccluded)
  {
    ezUInt32 uiNumLineVertices = pData->m_lineOccludedVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("OCCLUDED_PASS", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_lineOccludedVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Lines].GetNewBuffer();
        const ezUInt32 uiNumLineVerticesInBatch = ezMath::Min<ezUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        EZ_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_VertexAttributes), ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("OCCLUDED_PASS", "FALSE");

  // 3D Lines without occlusion
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

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_VertexAttributes), ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

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
        bindGroupRenderPass.BindBuffer("boxData", hBuffer);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch).IgnoreResult();

        uiNumLineBoxes -= uiNumLineBoxesInBatch;
        pLineBoxData += BOXES_PER_BATCH;
      }
    }
  }

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
        bindGroupRenderPass.BindBuffer("boxData", hBuffer);
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

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_VertexAttributes), ezGALPrimitiveTopology::Triangles, uiNumTriangleVerticesInBatch / 3);

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
      auto hTexture = itTex.Key();
      const auto format = pDevice->GetTexture(hTexture)->GetDescription().m_Format;
      const bool bMonochrome = ezGALResourceFormat::GetChannelCount(format) == 1;

      bindGroupRenderPass.BindTexture("BaseTexture", hTexture);

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

          renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_TexVertexAttributes), ezGALPrimitiveTopology::Triangles, uiNumVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNumVertices -= uiNumVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // Thick lines don't write depth, thus they are rendered after all other debug geometry, otherwise it would cover them.
  // The occluded ones come first, so that the depth tested ones override them where they are visible.
  if (cvar_DebugRenderOccluded)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("OCCLUDED_PASS", "TRUE");
    RenderThickLines(renderViewContext, pData->m_thickLinesOccluded);
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("OCCLUDED_PASS", "FALSE");
  }

  RenderThickLines(renderViewContext, pData->m_thickLines);

  // Text
  {
    pData->m_glyphs.Clear();

    const float fRenderScale = renderViewContext.m_pViewData->m_fRenderScale;

    for (auto& textLine : pData->m_textLines3D)
    {
      ezVec3 screenPos;
      if (renderViewContext.m_pViewData->ComputeScreenSpacePos(textLine.m_position, screenPos).Succeeded() && screenPos.z > 0.0f && screenPos.z < 1.0f)
      {
        renderViewContext.m_pViewData->ConvertScreenNormalizedPosToScaledPixelPos(screenPos);

        textLine.m_topLeftCorner.x = ezMath::Round(screenPos.x + textLine.m_topLeftCorner.x * fRenderScale);
        textLine.m_topLeftCorner.y = ezMath::Round(screenPos.y + textLine.m_topLeftCorner.y * fRenderScale);

        AppendGlyphs(pData->m_glyphs, textLine, fRenderScale);
      }
    }

    ezUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);

      bindGroupRenderPass.BindTexture("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Glyphs].GetNewBuffer();
        const ezUInt32 uiNumGlyphsInBatch = ezMath::Min<ezUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        bindGroupRenderPass.BindBuffer("glyphData", hBuffer);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

void ezDebugRendererInternal::RenderScreenSpace(const ezRenderViewContext& renderViewContext)
{
  EZ_PROFILE_SCOPE("ezDebugRendererInternal::RenderScreenSpace");

  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

static void RenderInternalScreenSpace(const ezDebugRendererContext& context, const ezRenderViewContext& renderViewContext)
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

    int lineHeight = (int)ezDebugRenderer::GetTextLineHeight();

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

        pos.y += offset * ezDebugRenderer::Draw2DText(context, cd[i].m_text.GetData(), pos, cd[i].m_color, 16, ha[corner], va[corner]);
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

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_VertexAttributes), ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 2D triangles
  {
    ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
    for (auto itTex = pData->m_texTriangle2DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      auto hTexture = itTex.Key();
      const auto format = pDevice->GetTexture(hTexture)->GetDescription().m_Format;
      const bool bMonochrome = ezGALResourceFormat::GetChannelCount(format) == 1;

      bindGroupRenderPass.BindTexture("BaseTexture", hTexture);

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

          renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_TexVertexAttributes), ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

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

        renderViewContext.m_pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hBuffer, 1), ezGALBufferHandle(), ezMakeArrayPtr(s_VertexAttributes), ezGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // Text
  {
    ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
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
      bindGroupRenderPass.BindTexture("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        ezGALBufferHandle hBuffer = s_DataBuffer[BufferType::Glyphs].GetNewBuffer();
        const ezUInt32 uiNumGlyphsInBatch = ezMath::Min<ezUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        bindGroupRenderPass.BindBuffer("glyphData", hBuffer);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

static void OnEngineStartup()
{
  {
    ezGeometry geom;
    geom.AddLineBox(ezVec3(2.0f));

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezMeshVertexStreamType::Position);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Lines);

    s_hLineBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    // BEGIN-DOCS-CODE-SNIPPET: resource-management-create
    ezGeometry geom;
    geom.AddBox(ezVec3(2.0f), false);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezMeshVertexStreamType::Position);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
    // END-DOCS-CODE-SNIPPET
  }

  {
    {
      ezGALVertexAttribute& va = s_VertexAttributes[0];
      va.m_eSemantic = ezGALVertexAttributeSemantic::Position;
      va.m_eFormat = ezGALResourceFormat::XYZFloat;
      va.m_uiOffset = 0;
      va.m_uiVertexBufferSlot = 0;
    }

    {
      ezGALVertexAttribute& va = s_VertexAttributes[1];
      va.m_eSemantic = ezGALVertexAttributeSemantic::Color0;
      va.m_eFormat = ezGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = 12;
      va.m_uiVertexBufferSlot = 0;
    }
  }

  {
    s_TexVertexAttributes[0] = s_VertexAttributes[0];
    s_TexVertexAttributes[1] = s_VertexAttributes[1];

    {
      ezGALVertexAttribute& va = s_TexVertexAttributes[2];
      va.m_eSemantic = ezGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = ezGALResourceFormat::XYFloat;
      va.m_uiOffset = 16;
      va.m_uiVertexBufferSlot = 0;
    }
  }

  {
    ezImage debugFontImage;
    ezGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);
    debugFontImage.Convert(ezImageFormat::R8_UNORM).AssertSuccess();

    ezGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = debugFontImage.GetByteBlobPtr();
    memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(debugFontImage.GetDepthPitch());

    ezTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = debugFontImage.GetWidth();
    desc.m_DescGAL.m_uiHeight = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RUByteNormalized;
    desc.m_DescGAL.m_ResourceAccess.m_bImmutable = true;
    desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = ezResourceManager::CreateResource<ezTexture2DResource>("DebugFontTexture", std::move(desc));
  }

  s_hDebugGeometryShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugGeometry.ezShader");
  s_hDebugPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugPrimitive.ezShader");
  s_hDebugTexturedPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugTexturedPrimitive.ezShader");
  s_hDebugTextShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugText.ezShader");
  s_hDebugThickLinesShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugThickLines.ezShader");

  ezRenderWorld::GetRenderEvent().AddEventHandler(&OnRenderEvent);
}

static void OnEngineShutdown()
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
  s_hDebugThickLinesShader.Invalidate();

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

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawLineCapsuleZ, In, "World", In, "Position", In, "Length", In, "Radius", In, "Color", In, "Transform")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(0.5f)),
      new ezFunctionArgumentAttributes(4, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawLineCylinderZ, In, "World", In, "Position", In, "Length", In, "Radius", In, "Color", In, "Transform")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(0.5f)),
      new ezFunctionArgumentAttributes(4, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawCylinder, In, "World", In, "Position", In, "RadiusStart", In, "RadiusEnd", In, "Length", In, "SolidColor", In, "LineColor", In, "Transform", In, "Caps", In, "Axis")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(0.5f)),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(0.5f)),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(5, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(6, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(8, new ezDefaultValueAttribute(true))),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawArrow, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform", In, "ForwardAxis")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(ezVec3(1, 0, 0)))),

    EZ_SCRIPT_FUNCTION_PROPERTY(Draw2DRectangle, In, "World", In, "Position", In, "Size", In, "Color")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(Draw2DLineRectangle, In, "World", In, "Position", In, "Size", In, "Color")->AddAttributes(
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

    EZ_SCRIPT_FUNCTION_PROPERTY(AddPersistentLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor", In, "Duration")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(4, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1)))),

    EZ_SCRIPT_FUNCTION_PROPERTY(AddPersistentCylinder, In, "World", In, "Position", In, "RadiusStart", In, "RadiusEnd", In, "Length", In, "SolidColor", In, "LineColor", In, "Transform", In, "Duration", In, "Caps", In, "Axis")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(0.5f)),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(0.5f)),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(5, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(6, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(8, new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1))))->AddAttributes(
      new ezFunctionArgumentAttributes(9, new ezDefaultValueAttribute(true))),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezExposeColorAlphaAttribute()),
      new ezFunctionArgumentAttributes(4, new ezExposeColorAlphaAttribute())),

    EZ_SCRIPT_FUNCTION_PROPERTY(DrawThickLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor", In, "Thickness")->AddAttributes(
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(4.0f))),

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
  ezDebugRenderer::DrawCross(pWorld, vPosition, fSize, color, 0.0f, transform);
}

// static
void ezScriptExtensionClass_Debug::DrawLineBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform)
{
  ezDebugRenderer::DrawLineBox(pWorld, ezBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, 0.0f, transform);
}

// static
void ezScriptExtensionClass_Debug::DrawLineSphere(const ezWorld* pWorld, const ezVec3& vPosition, float fRadius, const ezColor& color, const ezTransform& transform)
{
  ezDebugRenderer::DrawLineSphere(pWorld, ezBoundingSphere::MakeFromCenterAndRadius(vPosition, fRadius), color, 0.0f, transform);
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
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::AddPersistentCross(pWorld, fSize, color, 0.0f, t, duration);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentLineBox(const ezWorld* pWorld, const ezVec3& vPosition, const ezVec3& vHalfExtents, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::AddPersistentLineBox(pWorld, vHalfExtents, color, 0.0f, t, duration);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentLineSphere(const ezWorld* pWorld, const ezVec3& vPosition, float fRadius, const ezColor& color, const ezTransform& transform, ezTime duration)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::AddPersistentLineSphere(pWorld, fRadius, color, 0.0f, t, duration);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor, ezTime duration)
{
  ezDebugRendererLine line[1];
  line[0].m_start = vStart;
  line[0].m_end = vEnd;
  line[0].m_startColor = startColor;
  line[0].m_endColor = endColor;

  ezDebugRenderer::AddPersistentLines(pWorld, line, ezColor::White, 0.0f, ezTransform::MakeIdentity(), duration);
}

// static
void ezScriptExtensionClass_Debug::AddPersistentCylinder(const ezWorld* pWorld, const ezVec3& vPosition, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, ezTime duration, bool bCaps, ezEnum<ezBasisAxis> cylinderAxis)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::AddPersistentCylinder(pWorld, fRadiusStart, fRadiusEnd, fLength, solidColor, lineColor, 0.0f, t, duration, bCaps, bCaps, cylinderAxis);
}

// static
void ezScriptExtensionClass_Debug::DrawLineCapsuleZ(const ezWorld* pWorld, const ezVec3& vPosition, float fLength, float fRadius, const ezColor& color, const ezTransform& transform)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::DrawLineCapsuleZ(pWorld, fLength, fRadius, color, 0.0f, t);
}

// static
void ezScriptExtensionClass_Debug::DrawLineCylinderZ(const ezWorld* pWorld, const ezVec3& vPosition, float fLength, float fRadius, const ezColor& color, const ezTransform& transform)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::DrawLineCylinderZ(pWorld, fLength, fRadius, color, 0.0f, t);
}

// static
void ezScriptExtensionClass_Debug::DrawCylinder(const ezWorld* pWorld, const ezVec3& vPosition, float fRadiusStart, float fRadiusEnd, float fLength, const ezColor& solidColor, const ezColor& lineColor, const ezTransform& transform, bool bCaps, ezEnum<ezBasisAxis> cylinderAxis)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::DrawCylinder(pWorld, fRadiusStart, fRadiusEnd, fLength, solidColor, lineColor, 0.0f, t, bCaps, bCaps, cylinderAxis);
}

// static
void ezScriptExtensionClass_Debug::DrawArrow(const ezWorld* pWorld, const ezVec3& vPosition, float fSize, const ezColor& color, const ezTransform& transform, const ezVec3& vForwardAxis)
{
  ezTransform t = transform;
  t.m_vPosition = transform.TransformPosition(vPosition);

  ezDebugRenderer::DrawArrow(pWorld, fSize, color, 0.0f, t, vForwardAxis);
}

// static
void ezScriptExtensionClass_Debug::Draw2DRectangle(const ezWorld* pWorld, const ezVec3& vPositionInPixel, const ezVec3& vSizeInPixel, const ezColor& color)
{
  ezDebugRenderer::Draw2DRectangle(pWorld, ezRectFloat(vPositionInPixel.x, vPositionInPixel.y, vSizeInPixel.x, vSizeInPixel.y), vPositionInPixel.z, color);
}

// static
void ezScriptExtensionClass_Debug::Draw2DLineRectangle(const ezWorld* pWorld, const ezVec3& vPositionInPixel, const ezVec3& vSizeInPixel, const ezColor& color)
{
  ezDebugRenderer::Draw2DLineRectangle(pWorld, ezRectFloat(vPositionInPixel.x, vPositionInPixel.y, vSizeInPixel.x, vSizeInPixel.y), vPositionInPixel.z, color);
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

void ezScriptExtensionClass_Debug::DrawThickLine(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, const ezColor& startColor, const ezColor& endColor, float fThickness)
{
  ezDebugRendererLine line[1];
  line[0].m_start = vStart;
  line[0].m_end = vEnd;
  line[0].m_startColor = startColor;
  line[0].m_endColor = endColor;

  ezDebugRenderer::DrawLines(pWorld, line, ezColor::White, fThickness);
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
