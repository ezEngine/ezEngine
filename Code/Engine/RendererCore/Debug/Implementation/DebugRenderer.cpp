#include <RendererCore/PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ShaderResource.h>

#include <CoreUtils/Geometry/GeomUtils.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

namespace
{
  struct Vertex
  {
    ezVec3 m_position;
    ezColorLinearUB m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(Vertex) == 16);

  struct BoxData
  {
    ezVec4 m_transform0;
    ezVec4 m_transform1;
    ezVec4 m_transform2;
    ezColor m_color;
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(BoxData) == 64);

  struct PerWorldData
  {
    ezDynamicArray<Vertex> m_lineVertices;
    ezDynamicArray<Vertex> m_triangleVertices;
    ezDynamicArray<BoxData> m_lineBoxes;
    ezDynamicArray<BoxData> m_solidBoxes;
  };

  static ezDynamicArray<PerWorldData*> s_PerWorldData[2];

  static ezMutex s_Mutex;

  PerWorldData& GetDataForExtraction(ezUInt32 uiWorldIndex)
  {
    const ezUInt32 uiDataIndex = ezRenderLoop::GetDataIndexForExtraction();

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
      Triangles,

      Count
    };
  };

  ezGALBufferHandle s_hDataBuffer[BufferType::Count];
    
  ezMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  ezMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  ezVertexDeclarationInfo s_VertexDeclarationInfo;
  ezShaderResourceHandle s_hDebugGeometryShader;
  ezShaderResourceHandle s_hDebugPrimitiveShader;

  static void CreateDataBuffer(BufferType::Enum bufferType, ezUInt32 uiStructSize, ezUInt32 uiNumData, void* pData)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiStructSize;
    desc.m_uiTotalSize = desc.m_uiStructSize * uiNumData;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;

    s_hDataBuffer[bufferType] = pDevice->CreateBuffer(desc, pData);
  }

  static void CreateVertexBuffer(BufferType::Enum bufferType, ezUInt32 uiVertexSize, ezUInt32 uiNumVertices, void* pData)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    s_hDataBuffer[bufferType] = pDevice->CreateVertexBuffer(uiVertexSize, uiNumVertices, pData);
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

//static
void ezDebugRenderer::Render(const ezRenderViewContext& renderViewContext)
{
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

  // Lines
  {
    const ezUInt32 uiNumLineVertices = pData->m_lineVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      DestroyBuffer(BufferType::Lines);
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex), uiNumLineVertices, pData->m_lineVertices.GetData());

      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Lines], ezGALBufferHandle(), &s_VertexDeclarationInfo, 
        ezGALPrimitiveTopology::Lines, uiNumLineVertices / 2);
      renderViewContext.m_pRenderContext->DrawMeshBuffer();
    }
  }

  // LineBoxes
  {
    const ezUInt32 uiNumLineBoxes = pData->m_lineBoxes.GetCount();
    if (uiNumLineBoxes != 0)
    {
      DestroyBuffer(BufferType::LineBoxes);
      CreateDataBuffer(BufferType::LineBoxes, sizeof(BoxData), uiNumLineBoxes, pData->m_lineBoxes.GetData());

      pGALContext->SetResourceView(ezGALShaderStage::VertexShader, 0, pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::LineBoxes]));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);
      renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxes);
    }
  }

  // SolidBoxes
  {
    const ezUInt32 uiNumSolidBoxes = pData->m_solidBoxes.GetCount();
    if (uiNumSolidBoxes != 0)
    {
      DestroyBuffer(BufferType::SolidBoxes);
      CreateDataBuffer(BufferType::SolidBoxes, sizeof(BoxData), uiNumSolidBoxes, pData->m_solidBoxes.GetData());

      pGALContext->SetResourceView(ezGALShaderStage::VertexShader, 0, pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::SolidBoxes]));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);
      renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumSolidBoxes);      
    }
  }
  
  // Triangles
  {
    const ezUInt32 uiNumTriangleVertices = pData->m_triangleVertices.GetCount();
    if (uiNumTriangleVertices != 0)
    {
      DestroyBuffer(BufferType::Triangles);
      CreateVertexBuffer(BufferType::Triangles, sizeof(Vertex), uiNumTriangleVertices, pData->m_triangleVertices.GetData());

      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles], ezGALBufferHandle(), &s_VertexDeclarationInfo, 
        ezGALPrimitiveTopology::Lines, uiNumTriangleVertices / 3);
      renderViewContext.m_pRenderContext->DrawMeshBuffer();
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

  s_hDebugGeometryShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugGeometry.ezShader");
  s_hDebugPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Debug/DebugPrimitive.ezShader");

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

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
}
