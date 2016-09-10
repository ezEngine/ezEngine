#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <Foundation/Logging/Log.h>


namespace ezModelImporter
{
  VertexDataStream::VertexDataStream(ezUInt32 uiNumElementsPerVertex, ezUInt32 uiNumTriangles)
    : m_uiNumElementsPerVertex(uiNumElementsPerVertex)
  {
    m_IndexToData.SetCount(uiNumTriangles * 3);
  }

  void VertexDataStream::ReserveData(ezUInt32 numExpectedValues)
  {
    // +1 for the zero entry at the start of the array.
    m_Data.Reserve((numExpectedValues + 1) * m_uiNumElementsPerVertex);
  }

  Mesh::Mesh()
    : HierarchyObject(ObjectHandle::MESH)
    , m_uiNextUnusedVertexIndex(0)
  {}

  Mesh::~Mesh()
  {
    for (auto it = m_VertexDataStreams.GetIterator(); it.IsValid(); ++it)
    {
      EZ_DEFAULT_DELETE(it.Value());
    }
  }

  VertexDataStream* Mesh::AddDataStream(ezGALVertexAttributeSemantic::Enum semantic, ezUInt32 uiNumElementsPerVertex)
  {
    VertexDataStream* existingStream = nullptr;
    if (!m_VertexDataStreams.TryGetValue(static_cast<ezUInt32>(semantic), existingStream))
    {
      m_VertexDataStreams.Insert(semantic, EZ_DEFAULT_NEW(VertexDataStream, uiNumElementsPerVertex, m_Triangles.GetCount()));
      return m_VertexDataStreams[semantic];
    }
    else
    {
      if (uiNumElementsPerVertex != existingStream->GetNumElementsPerVertex())
      {
        return nullptr;
      }
      return existingStream;
    }
  }

  VertexDataStream* Mesh::GetDataStream(ezGALVertexAttributeSemantic::Enum semantic)
  {
    VertexDataStream* out = nullptr;
    m_VertexDataStreams.TryGetValue(semantic, out);
    return out;
  }

  const VertexDataStream* Mesh::GetDataStream(ezGALVertexAttributeSemantic::Enum semantic) const
  {
    VertexDataStream* out = nullptr;
    m_VertexDataStreams.TryGetValue(semantic, out);
    return out;
  }

  void Mesh::AddTriangles(ezUInt32 num)
  {
    m_Triangles.Reserve(m_Triangles.GetCount() + num);
    for (ezUInt32 i = 0; i < num; ++i)
    {
      Triangle& tri = m_Triangles.ExpandAndGetRef();
      tri.m_Vertices[0].m_Value = m_uiNextUnusedVertexIndex; ++m_uiNextUnusedVertexIndex;
      tri.m_Vertices[1].m_Value = m_uiNextUnusedVertexIndex; ++m_uiNextUnusedVertexIndex;
      tri.m_Vertices[2].m_Value = m_uiNextUnusedVertexIndex; ++m_uiNextUnusedVertexIndex;
    }

    for (auto streamIt = m_VertexDataStreams.GetIterator(); streamIt.IsValid(); ++streamIt)
    {
      streamIt.Value()->m_IndexToData.Reserve(m_Triangles.GetCount() * 3);
      for (ezUInt32 i = 0; i < num * 3; ++i)
        streamIt.Value()->m_IndexToData.PushBack(VertexDataIndex());
    }
  }

  ezUInt32 Mesh::GetNumSubMeshes() const
  {
    return m_SubMeshes.GetCount();
  }

  const SubMesh& Mesh::GetSubMesh(ezUInt32 idx) const
  {
    return m_SubMeshes[idx];
  }

  SubMesh& Mesh::GetSubMesh(ezUInt32 idx)
  {
    return m_SubMeshes[idx];
  }

  void Mesh::AddSubMesh(SubMesh& mesh)
  {
    return m_SubMeshes.PushBack(mesh);
  }

  void Mesh::AddData(const Mesh& mesh)
  {
    // Create new triangles.
    ezUInt32 oldTriangleCount = GetNumTriangles();
    AddTriangles(mesh.GetNumTriangles());

    ezArrayPtr<const Mesh::Triangle> sourceTriangles = mesh.GetTriangles();
    ezArrayPtr<const Mesh::Triangle> targetTriangles = GetTriangles().GetSubArray(oldTriangleCount);
    EZ_ASSERT_DEBUG(sourceTriangles.GetCount() == targetTriangles.GetCount(), "Something is wrong with triangle allocation!");

    for (auto it = mesh.m_VertexDataStreams.GetIterator(); it.IsValid(); ++it)
    {
      const VertexDataStream* sourceStream = it.Value();
      VertexDataStream* targetStream = AddDataStream(static_cast<ezGALVertexAttributeSemantic::Enum>(it.Key()), sourceStream->GetNumElementsPerVertex());
      if (!targetStream)
      {
        ezLog::SeriousWarning("Cannot merge mesh %s properly since it has a vertex data stream with semantic %i that uses %i elements instead of %i which is used by the merge target. Skipping this data stream.",
                              mesh.m_Name.GetData(), it.Key(), sourceStream->GetNumElementsPerVertex(), targetStream->GetNumElementsPerVertex());
        continue;
      }

      // Copy data.
      ezUInt32 targetBaseDataIndex = targetStream->m_Data.GetCount();
      targetStream->m_Data.PushBackRange(sourceStream->m_Data);

      // Set mapping
      for (ezUInt32 tri = 0; tri < sourceTriangles.GetCount(); ++tri)
      {
        for (int v = 0; v < 3; ++v)
        {
          VertexDataIndex sourceDataIndex = sourceStream->GetDataIndex(sourceTriangles[tri].m_Vertices[v]);
          if (sourceDataIndex.IsValid())
            targetStream->SetDataIndex(targetTriangles[tri].m_Vertices[v], targetBaseDataIndex + sourceDataIndex.GetValue());
        }
      }
    }

    // Add submeshes.
    ezUInt32 oldSubMeshCount = m_SubMeshes.GetCount();
    m_SubMeshes.PushBackRange(mesh.m_SubMeshes);
    for (ezUInt32 i = oldSubMeshCount; i < m_SubMeshes.GetCount(); ++i)
    {
      m_SubMeshes[i].m_uiFirstTriangle += oldTriangleCount;
    }
  }
}
