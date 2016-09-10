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
    // A few checks for meaningful element count.
    switch (semantic)
    {
    case ezGALVertexAttributeSemantic::Position:
      EZ_ASSERT_DEBUG(uiNumElementsPerVertex == 3, "Position vertex streams should always have exactly 3 elements.");
      break;
    case ezGALVertexAttributeSemantic::Normal:
      EZ_ASSERT_DEBUG(uiNumElementsPerVertex == 3, "Normal vertex streams should always have exactly 3 elements.");
      break;
    case ezGALVertexAttributeSemantic::Tangent:
      EZ_ASSERT_DEBUG(uiNumElementsPerVertex == 3, "Tangent vertex streams should always have exactly 3 elements.");
      break;
    case ezGALVertexAttributeSemantic::BiTangent:
      EZ_ASSERT_DEBUG(uiNumElementsPerVertex == 3, "BiTangent vertex streams should always have exactly 3 elements.");
      break;
    }

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

  void Mesh::ApplyTransform(const ezTransform& transform)
  {
    ezMat4 transformMat = transform.GetAsMat4();

    for (auto it = m_VertexDataStreams.GetIterator(); it.IsValid(); ++it)
    {
      VertexDataStream* sourceStream = it.Value();

      // Positions
      if (it.Key() == ezGALVertexAttributeSemantic::Position)
      {
        for (ezUInt32 i = 0; i < sourceStream->m_Data.GetCount(); i += 3)
        {
          ezVec3& pos = *reinterpret_cast<ezVec3*>(&sourceStream[i]);
          pos = transformMat.TransformPosition(pos);
        }
      }

      // Directions
      else if (it.Key() == ezGALVertexAttributeSemantic::Normal ||
               it.Key() == ezGALVertexAttributeSemantic::Tangent ||
               it.Key() == ezGALVertexAttributeSemantic::BiTangent)
      {
        for (ezUInt32 i = 0; i < sourceStream->m_Data.GetCount(); i += 3)
        {
          ezVec3& dir = *reinterpret_cast<ezVec3*>(&sourceStream[i]);
          dir = transformMat.TransformDirection(dir);
        }
      }
    }
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

  void Mesh::MergeSubMeshesWithSameMaterials()
  {
    // Find out which material maps to which submesh.
    // To make the result deterministic and change the order of submeshes/materials as little as possible, we cannot rely on maps or hashmaps
    bool anyDuplicate = false;
    typedef ezHybridArray<SubMesh*, 4> SubMeshBundle;
    ezDynamicArray<SubMeshBundle> subMeshBundles;
    for (int i = m_SubMeshes.GetCount() - 1; i >= 0; --i)
    {
      auto materialHandle = m_SubMeshes[i].m_Material;
      SubMeshBundle* existingBundle = nullptr;
      for (SubMeshBundle& bundle : subMeshBundles)
      {
        if (bundle[0]->m_Material == materialHandle)
        {
          existingBundle = &bundle;
          break;
        }
      }

      if(!existingBundle)
      {
        SubMeshBundle bundle;
        bundle.PushBack(&m_SubMeshes[i]);
        subMeshBundles.PushBack(bundle);
      }
      else
      {
        existingBundle->PushBack(&m_SubMeshes[i]);
       anyDuplicate = true;
      }
    }

    // Nothing to do?
    if (!anyDuplicate)
      return;

    // Rewrite triangle and submesh list.
    // Note that in the special case in which all triangles are already sorted correctly we perform really badly. Optimizing this however is a lot more code.
    ezDynamicArray<Triangle> trianglesNew;
    trianglesNew.Reserve(m_Triangles.GetCount());
    ezDynamicArray<SubMesh> subMeshesNew;
    subMeshesNew.Reserve(subMeshBundles.GetCount());
    for (SubMeshBundle& bundle : subMeshBundles)
    {
      SubMesh& newSubMesh = subMeshesNew.ExpandAndGetRef();
      newSubMesh.m_Material = bundle[0]->m_Material;
      newSubMesh.m_uiFirstTriangle = trianglesNew.GetCount();

      for (SubMesh* oldSubMesh : bundle)
      {
        newSubMesh.m_uiTriangleCount += oldSubMesh->m_uiTriangleCount;
        trianglesNew.PushBackRange(m_Triangles.GetArrayPtr().GetSubArray(oldSubMesh->m_uiFirstTriangle, oldSubMesh->m_uiTriangleCount));
      }
    }

    if (m_Triangles.GetCount() > trianglesNew.GetCount())
      ezLog::Warning("There were some triangles in submeshes of the mesh '%s' that were not referenced by any submesh. These triangles were discarded while merging submeshes.", m_Name.GetData());
    else if(m_Triangles.GetCount() < trianglesNew.GetCount())
      ezLog::Warning("There are submeshes in '%s' with overlapping triangle use. These triangles were duplicated while merging submeshes.", m_Name.GetData());

    m_Triangles = std::move(trianglesNew);
    m_SubMeshes = std::move(subMeshesNew);
  }
}
