#include <RendererCore/PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureResource.h>

#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Graphics/SimpleASCIIFont.h>

namespace
{
  struct EZ_ALIGN_16(Vertex)
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(Vertex) == 16);

  struct EZ_ALIGN_16(BoxData)
  {
    ezVec4 m_transform0;
    ezVec4 m_transform1;
    ezVec4 m_transform2;
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

  struct PerWorldData
  {
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_lineVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangleVertices;
    ezDynamicArray<Vertex, ezAlignedAllocatorWrapper> m_triangle2DVertices;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_lineBoxes;
    ezDynamicArray<BoxData, ezAlignedAllocatorWrapper> m_solidBoxes;
    ezDynamicArray<GlyphData, ezAlignedAllocatorWrapper> m_glyphs;
  };

  static ezDynamicArray<PerWorldData*> s_PerWorldData[2];

  static ezMutex s_Mutex;

  static ezUInt32 s_uiLastRenderedFrame;

  PerWorldData& GetDataForExtraction(ezUInt32 uiWorldIndex)
  {
    const ezUInt32 uiDataIndex = ezRenderLoop::IsRenderingThread() && (s_uiLastRenderedFrame != ezRenderLoop::GetFrameCounter()) ? 
      ezRenderLoop::GetDataIndexForRendering() : ezRenderLoop::GetDataIndexForExtraction();

    auto& perWorldData = s_PerWorldData[uiDataIndex];
    if (uiWorldIndex >= perWorldData.GetCount())
    {
      perWorldData.SetCount(uiWorldIndex + 1);
    }

    PerWorldData* pData = perWorldData[uiWorldIndex];
    if (pData == nullptr)
    {
      pData = EZ_DEFAULT_NEW(PerWorldData);
      perWorldData[uiWorldIndex] = pData;
    }

    return *pData;
  }

  void ClearRenderData(ezUInt32)
  {
    auto& perWorldData = s_PerWorldData[ezRenderLoop::GetDataIndexForRendering()];

    for (ezUInt32 i = 0; i < perWorldData.GetCount(); ++i)
    {
      PerWorldData* pData = perWorldData[i];
      if (pData)
      {
        pData->m_lineVertices.Clear();
        pData->m_lineBoxes.Clear();
        pData->m_solidBoxes.Clear();
        pData->m_triangleVertices.Clear();
        pData->m_triangle2DVertices.Clear();
        pData->m_glyphs.Clear();
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
      Glyphs,

      Count
    };
  };

  ezGALBufferHandle s_hDataBuffer[BufferType::Count];
    
  ezMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  ezMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  ezVertexDeclarationInfo s_VertexDeclarationInfo;
  ezTextureResourceHandle s_hDebugFontTexture;

  ezShaderResourceHandle s_hDebugGeometryShader;
  ezShaderResourceHandle s_hDebugPrimitiveShader;
  ezShaderResourceHandle s_hDebugTextShader;

  enum
  {
    DEBUG_BUFFER_SIZE = 1024 * 256,
    BOXES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
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

  template <typename T>
  static void UpdateBuffer(ezGALContext* pGALContext, BufferType::Enum bufferType, ezArrayPtr<const T> pData)
  {
    pGALContext->UpdateBuffer(s_hDataBuffer[bufferType], 0, ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(pData.GetPtr()), pData.GetCount() * sizeof(T)));
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
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, DebugRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_ENGINE_STARTUP
  {
    ezDebugRenderer::OnEngineStartup();
  }

  ON_ENGINE_SHUTDOWN
  {
    ezDebugRenderer::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

//static 
void ezDebugRenderer::DrawLines(const ezWorld* pWorld, ezArrayPtr<Line> lines, const ezColor& color)
{
  DrawLines(pWorld->GetIndex(), lines, color);
}

//static 
void ezDebugRenderer::DrawLines(ezUInt32 uiWorldIndex, ezArrayPtr<Line> lines, const ezColor& color)
{
  if (lines.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(uiWorldIndex);

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

//static
void ezDebugRenderer::DrawLineBox(const ezWorld* pWorld, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform)
{
  DrawLineBox(pWorld->GetIndex(), box, color, transform);
}

//static
void ezDebugRenderer::DrawLineBox(ezUInt32 uiWorldIndex, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(uiWorldIndex);
  
  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();
  
  ezMat3 scalingMat; scalingMat.SetScalingMatrix(box.GetHalfExtents());
  ezTransform boxTransform(box.GetCenter(), scalingMat);

  ezMat4 matrix = (transform * boxTransform).GetAsMat4();

  boxData.m_transform0 = matrix.GetRow(0);
  boxData.m_transform1 = matrix.GetRow(1);
  boxData.m_transform2 = matrix.GetRow(2);
  boxData.m_color = color;
}

//static
void ezDebugRenderer::DrawLineBoxCorners(const ezWorld* pWorld, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform)
{
  DrawLineBoxCorners(pWorld->GetIndex(), box, fCornerFraction, color, transform);
}

//static
void ezDebugRenderer::DrawLineBoxCorners(ezUInt32 uiWorldIndex, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform)
{
  fCornerFraction = ezMath::Clamp(fCornerFraction, 0.0f, 1.0f) * 0.5f;

  ezVec3 corners[8];
  box.GetCorners(corners);

  ezMat4 matTransform = transform.GetAsMat4();
  for (ezUInt32 i = 0; i < 8; ++i)
  {
    corners[i] = matTransform.TransformPosition(corners[i]);
  }

  ezVec3 edgeEnds[12];
  edgeEnds[0]  = corners[1]; // 0 -> 1
  edgeEnds[1]  = corners[3]; // 1 -> 3
  edgeEnds[2]  = corners[0]; // 2 -> 0
  edgeEnds[3]  = corners[2]; // 3 -> 2
  edgeEnds[4]  = corners[5]; // 4 -> 5
  edgeEnds[5]  = corners[7]; // 5 -> 7
  edgeEnds[6]  = corners[4]; // 6 -> 4
  edgeEnds[7]  = corners[6]; // 7 -> 6
  edgeEnds[8]  = corners[4]; // 0 -> 4
  edgeEnds[9]  = corners[5]; // 1 -> 5
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

  DrawLines(uiWorldIndex, lines, color);
}

//static
void ezDebugRenderer::DrawSolidBox(const ezWorld* pWorld, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform)
{
  DrawSolidBox(pWorld->GetIndex(), box, color, transform);
}

//static
void ezDebugRenderer::DrawSolidBox(ezUInt32 uiWorldIndex, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform)
{
  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(uiWorldIndex);

  auto& boxData = data.m_solidBoxes.ExpandAndGetRef();

  ezMat3 scalingMat; scalingMat.SetScalingMatrix(box.GetHalfExtents());
  ezTransform boxTransform(box.GetCenter(), scalingMat);

  ezMat4 matrix = (transform * boxTransform).GetAsMat4();

  boxData.m_transform0 = matrix.GetRow(0);
  boxData.m_transform1 = matrix.GetRow(1);
  boxData.m_transform2 = matrix.GetRow(2);
  boxData.m_color = color;
}

//static
void ezDebugRenderer::DrawSolidTriangles(const ezWorld* pWorld, ezArrayPtr<Triangle> triangles, const ezColor& color)
{
  DrawSolidTriangles(pWorld->GetIndex(), triangles, color);
}

//static
void ezDebugRenderer::DrawSolidTriangles(ezUInt32 uiWorldIndex, ezArrayPtr<Triangle> triangles, const ezColor& color)
{
  if (triangles.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(uiWorldIndex);

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

void ezDebugRenderer::Draw2DRectangle(const ezWorld* pWorld, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color)
{
  Draw2DRectangle(pWorld->GetIndex(), rectInPixel, fDepth, color);
}

void ezDebugRenderer::Draw2DRectangle(ezUInt32 uiWorldIndex, const ezRectFloat& rectInPixel, float fDepth, const ezColor& color)
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

  auto& data = GetDataForExtraction(uiWorldIndex);
 
  data.m_triangle2DVertices.PushBackRange(ezMakeArrayPtr(vertices));
}

void ezDebugRenderer::DrawText(const ezWorld* pWorld, const ezStringView& text, const ezVec2I32& topLeftCornerInPixel, const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/)
{
  DrawText(pWorld->GetIndex(), text, topLeftCornerInPixel, color, uiSizeInPixel);
}

void ezDebugRenderer::DrawText(ezUInt32 uiWorldIndex, const ezStringView& text, const ezVec2I32& topLeftCornerInPixel, const ezColor& color, ezUInt32 uiSizeInPixel /*= 16*/)
{
  if (text.IsEmpty())
    return;

  EZ_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(uiWorldIndex);

  float fSizeInPixel = (float)uiSizeInPixel;
  ezVec2 currentPos((float)topLeftCornerInPixel.x, (float)topLeftCornerInPixel.y);

  for (ezUInt32 uiCharacter : text)
  {
    auto& glyphData = data.m_glyphs.ExpandAndGetRef();
    glyphData.m_topLeftCorner = currentPos;
    glyphData.m_color = color;
    glyphData.m_glyphIndex = uiCharacter < 128 ? uiCharacter : 0;
    glyphData.m_sizeInPixel = (ezUInt16)uiSizeInPixel;

    // Glyphs only use 10x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
    currentPos.x += ezMath::Ceil(fSizeInPixel * (10.0f / 16.0f));
  }
}

//static
void ezDebugRenderer::Render(const ezRenderViewContext& renderViewContext)
{
  s_uiLastRenderedFrame = ezRenderLoop::GetFrameCounter();

  const ezUInt32 uiWorldIndex = renderViewContext.m_uiWorldIndex;

  auto& perWorldData = s_PerWorldData[ezRenderLoop::GetDataIndexForRendering()];
  if (uiWorldIndex >= perWorldData.GetCount())
  {
    return;
  }

  PerWorldData* pData = perWorldData[uiWorldIndex];
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
      renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "boxData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::SolidBoxes]));
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);

      const BoxData* pSolidBoxData = pData->m_solidBoxes.GetData();
      while (uiNumSolidBoxes > 0)
      {
        const ezUInt32 uiNumSolidBoxesInBatch = ezMath::Min<ezUInt32>(uiNumSolidBoxes, BOXES_PER_BATCH);
        UpdateBuffer(pGALContext, BufferType::SolidBoxes, ezMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch));

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
        UpdateBuffer(pGALContext, BufferType::Triangles3D, ezMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch));

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles3D], ezGALBufferHandle(), &s_VertexDeclarationInfo,
          ezGALPrimitiveTopology::Triangles, uiNumTriangleVerticesInBatch / 3);
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
        UpdateBuffer(pGALContext, BufferType::Lines, ezMakeArrayPtr(pLineData, uiNumLineVerticesInBatch));

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
      renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "boxData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::LineBoxes]));
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);

      const BoxData* pLineBoxData = pData->m_lineBoxes.GetData();
      while (uiNumLineBoxes > 0)
      {
        const ezUInt32 uiNumLineBoxesInBatch = ezMath::Min<ezUInt32>(uiNumLineBoxes, BOXES_PER_BATCH);
        UpdateBuffer(pGALContext, BufferType::LineBoxes, ezMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch));

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
        UpdateBuffer(pGALContext, BufferType::Triangles2D, ezMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch));

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles2D], ezGALBufferHandle(), &s_VertexDeclarationInfo,
          ezGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);
        renderViewContext.m_pRenderContext->DrawMeshBuffer();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Text
  {
    ezUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);
      renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "glyphData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::Glyphs]));
      renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "FontTexture", s_hDebugFontTexture);
      
      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        const ezUInt32 uiNumGlyphsInBatch = ezMath::Min<ezUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        UpdateBuffer(pGALContext, BufferType::Glyphs, ezMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch));

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

    s_hLineBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugLineBox", desc, "Mesh for Rendering Debug Line Boxes");
  }

  {
    ezGeometry geom;
    geom.AddBox(ezVec3(2.0f), ezColor::White);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("DebugSolidBox", desc, "Mesh for Rendering Debug Solid Boxes");
  }

  
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

  {
    ezImage debugFontImage;
    ezGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);

    ezGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = debugFontImage.GetDataPointer<ezUInt8>();
    memoryDesc.m_uiRowPitch = debugFontImage.GetRowPitch();
    memoryDesc.m_uiSlicePitch = debugFontImage.GetDepthPitch();

    ezTextureResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = debugFontImage.GetWidth();
    desc.m_DescGAL.m_uiHeight = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = ezResourceManager::CreateResource<ezTextureResource>("DebugFontTexture", desc);
  }

  s_hDebugGeometryShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugGeometry.ezShader");
  s_hDebugPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugPrimitive.ezShader");
  s_hDebugTextShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugText.ezShader");

  ezRenderLoop::s_EndFrameEvent.AddEventHandler(&ClearRenderData);
}

void ezDebugRenderer::OnEngineShutdown()
{
  ezRenderLoop::s_EndFrameEvent.RemoveEventHandler(&ClearRenderData);

  for (ezUInt32 i = 0; i < BufferType::Count; ++i)
  {
    DestroyBuffer(static_cast<BufferType::Enum>(i));    
  }

  s_hLineBoxMeshBuffer.Invalidate();
  s_hSolidBoxMeshBuffer.Invalidate();
  s_hDebugFontTexture.Invalidate();

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
  s_hDebugTextShader.Invalidate();
}
