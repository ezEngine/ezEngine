#include <PCH.h>

#include <Console/ConsoleFunction.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Utils/ExtractWorldGeoUtil.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/RenderMeshComponent.h>

namespace ezWorldUtils
{
  ezResult ExtractWorldGeometry_RenderMesh(const ezWorld& world, Geometry& geo, const ezRenderMeshComponent& component)
  {
    const char* szMesh = component.GetMeshFile();

    EZ_LOG_BLOCK("ExtractWorldGeometry_RenderMesh", szMesh);

    ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(szMesh);

    ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::NoFallback);

    if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      ezLog::Warning("Failed to retrieve CPU mesh");
      return EZ_FAILURE;
    }

    const auto& mb = pCpuMesh->GetDescriptor().MeshBufferDesc();

    if (mb.GetTopology() != ezGALPrimitiveTopology::Triangles || mb.GetPrimitiveCount() == 0 || !mb.HasIndexBuffer())
    {
      ezLog::Warning("Unsupported CPU mesh topology {0}", (int)mb.GetTopology());
      return EZ_FAILURE;
    }

    const ezTransform transform = component.GetOwner()->GetGlobalTransform();

    const ezVertexDeclarationInfo& vdi = mb.GetVertexDeclaration();
    const ezUInt8* pRawVertexData = mb.GetVertexBufferData().GetData();

    const float* pPositions = nullptr;
    ezUInt32 uiElementStride = 0;

    for (ezUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
    {
      uiElementStride += vdi.m_VertexStreams[vs].m_uiElementSize;

      if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Position)
      {
        if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
        {
          ezLog::Warning("Unsupported CPU mesh vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
          return EZ_FAILURE; // other position formats are not supported
        }

        pPositions = (const float*)(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
      }
    }

    if (pPositions == nullptr)
    {
      ezLog::Warning("No position stream found in CPU mesh");
      return EZ_FAILURE;
    }

    // remember the vertex index at the start
    const ezUInt32 uiVertexIdxOffset = geo.m_Vertices.GetCount();

    // write out all vertices
    for (ezUInt32 i = 0; i < mb.GetVertexCount(); ++i)
    {
      const ezVec3 pos(pPositions[0], pPositions[1], pPositions[2]);
      pPositions = ezMemoryUtils::AddByteOffsetConst(pPositions, uiElementStride);

      auto& vert = geo.m_Vertices.ExpandAndGetRef();
      vert.m_Position = transform * pos;
      // vert.m_TexCoord.SetZero();
    }

    if (mb.Uses32BitIndices())
    {
      const ezUInt32* pIndices = reinterpret_cast<const ezUInt32*>(mb.GetIndexBufferData().GetData());

      for (ezUInt32 p = 0; p < mb.GetPrimitiveCount(); ++p)
      {
        auto& tri = geo.m_Triangles.ExpandAndGetRef();
        tri.m_VertexIndices[0] = uiVertexIdxOffset + pIndices[p * 3 + 0];
        tri.m_VertexIndices[1] = uiVertexIdxOffset + pIndices[p * 3 + 1];
        tri.m_VertexIndices[2] = uiVertexIdxOffset + pIndices[p * 3 + 2];
      }
    }
    else
    {
      const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(mb.GetIndexBufferData().GetData());

      for (ezUInt32 p = 0; p < mb.GetPrimitiveCount(); ++p)
      {
        auto& tri = geo.m_Triangles.ExpandAndGetRef();
        tri.m_VertexIndices[0] = uiVertexIdxOffset + pIndices[p * 3 + 0];
        tri.m_VertexIndices[1] = uiVertexIdxOffset + pIndices[p * 3 + 1];
        tri.m_VertexIndices[2] = uiVertexIdxOffset + pIndices[p * 3 + 2];
      }
    }

    return EZ_SUCCESS;
  }

  void ExtractWorldGeometry(const ezWorld& world, Geometry& geo)
  {
    EZ_LOCK(world.GetReadMarker());

    ezDeque<ezGameObjectHandle> selection;

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      selection.PushBack(it->GetHandle());
    }

    ExtractWorldGeometry(world, geo, selection);
  }

  void ExtractWorldGeometry(const ezWorld& world, Geometry& geo, const ezDeque<ezGameObjectHandle>& selection)
  {
    EZ_LOCK(world.GetReadMarker());

    EZ_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

    for (ezGameObjectHandle hObject : selection)
    {
      const ezGameObject* pObject;
      if (!world.TryGetObject(hObject, pObject))
        continue;

      for (const ezComponent* pComp : pObject->GetComponents())
      {
        if (const ezRenderMeshComponent* pMesh = ezDynamicCast<const ezRenderMeshComponent*>(pComp))
        {
          ExtractWorldGeometry_RenderMesh(world, geo, *pMesh);
        }
      }
    }
  }

  void WriteWorldGeometryToOBJ(const Geometry& geo, const char* szFile)
  {
    EZ_LOG_BLOCK("Write World Geometry to OBJ", szFile);

    ezFileWriter file;
    if (file.Open(szFile).Failed())
    {
      ezLog::Error("Failed to open file for writing: '{0}'", szFile);
      return;
    }

    ezStringBuilder line;

    line.Format("\n\n# {0} vertices\n", geo.m_Vertices.GetCount());
    file.WriteBytes(line.GetData(), line.GetElementCount());

    for (ezUInt32 i = 0; i < geo.m_Vertices.GetCount(); ++i)
    {
      line.Format("v {0} {1} {2}\n", ezArgF(geo.m_Vertices[i].m_Position.x, 8), ezArgF(geo.m_Vertices[i].m_Position.y, 8),
                  ezArgF(geo.m_Vertices[i].m_Position.z, 8));

      file.WriteBytes(line.GetData(), line.GetElementCount());
    }

    line.Format("\n\n# {0} triangles\n", geo.m_Triangles.GetCount());
    file.WriteBytes(line.GetData(), line.GetElementCount());

    for (ezUInt32 i = 0; i < geo.m_Triangles.GetCount(); ++i)
    {
      line.Format("f {0} {1} {2}\n", geo.m_Triangles[i].m_VertexIndices[0] + 1, geo.m_Triangles[i].m_VertexIndices[1] + 1,
                  geo.m_Triangles[i].m_VertexIndices[2] + 1);

      file.WriteBytes(line.GetData(), line.GetElementCount());
    }

    ezLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
  }

  //void ExtractAllWorldGeoms()
  //{
  //  EZ_LOG_BLOCK("Extract All World Geometries");

  //  for (ezUInt32 w = 0; w < ezWorld::GetWorldCount(); ++w)
  //  {
  //    Geometry geo;
  //    ExtractWorldGeometry(*ezWorld::GetWorld(w), geo);

  //    ezStringBuilder fileName;
  //    fileName.Format(":appdata/ExtractedWorlds/world{0}.obj", w);
  //    WriteWorldGeometryToOBJ(geo, fileName);
  //  }
  //}

  //ezConsoleFunction<void()> g_ExtractWorldGeometryConFunc("ExtractWorldGeometry", "ForFunAndProfit", &ExtractAllWorldGeoms);

} // namespace ezWorldUtils
