#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <meshoptimizer/meshoptimizer.h>

namespace ezModelImporter2
{
  namespace
  {
    /// Reorders the triangles of an imported mesh for GPU vertex cache efficiency and its vertices for fetch locality.
    ///
    /// Triangles are only reordered within a sub-mesh, so that the index range of every sub-mesh stays valid.
    /// The vertex reordering is done across the entire mesh buffer and all indices are remapped accordingly.
    /// Only indexed triangle meshes are touched, everything else is left as it is.
    void OptimizeMeshForRendering(ezMeshResourceDescriptor& ref_desc)
    {
      ezMeshBufferResourceDescriptor& mb = ref_desc.MeshBufferDesc();

      if (mb.GetTopology() != ezGALPrimitiveTopology::Triangles || !mb.HasIndexBuffer())
        return;

      const ezUInt32 uiVertexCount = mb.GetVertexCount();
      const ezUInt32 uiIndexCount = mb.GetPrimitiveCount() * 3;

      if (uiVertexCount == 0 || uiIndexCount == 0)
        return;

      // meshoptimizer only works on 32 bit indices, so 16 bit index data is widened here and narrowed again at the end
      const bool bIndices32Bit = mb.Uses32BitIndices();

      ezTempArray<ezUInt32> indices;
      indices.SetCountUninitialized(uiIndexCount);

      {
        const auto& indexData = mb.GetIndexBufferData();

        if (bIndices32Bit)
        {
          ezMemoryUtils::Copy(indices.GetData(), reinterpret_cast<const ezUInt32*>(indexData.GetData()), uiIndexCount);
        }
        else
        {
          const ezUInt16* pSrc = reinterpret_cast<const ezUInt16*>(indexData.GetData());

          for (ezUInt32 i = 0; i < uiIndexCount; ++i)
          {
            indices[i] = pSrc[i];
          }
        }
      }

      // every sub-mesh is a separate draw call and thus has to be optimized on its own,
      // otherwise triangles would move out of the index range that the sub-mesh refers to
      for (const auto& subMesh : ref_desc.GetSubMeshes())
      {
        const ezUInt32 uiFirstIndex = subMesh.m_uiFirstPrimitive * 3;
        const ezUInt32 uiNumIndices = subMesh.m_uiPrimitiveCount * 3;

        if (uiNumIndices == 0 || uiFirstIndex + uiNumIndices > uiIndexCount)
          continue;

        meshopt_optimizeVertexCache(indices.GetData() + uiFirstIndex, indices.GetData() + uiFirstIndex, uiNumIndices, uiVertexCount);
      }

      // reorder the vertices in the order in which the (now optimized) index buffer references them
      {
        ezTempArray<ezUInt32> remap;
        remap.SetCountUninitialized(uiVertexCount);

        const ezUInt32 uiUniqueVertices = static_cast<ezUInt32>(meshopt_optimizeVertexFetchRemap(remap.GetData(), indices.GetData(), uiIndexCount, uiVertexCount));

        // if the mesh contains vertices that no triangle references, the remap table would shrink the vertex buffer.
        // that would require patching up everything that refers to vertex indices, so instead the vertex order is left alone in this case.
        if (uiUniqueVertices == uiVertexCount)
        {
          ezTempArray<ezUInt32> remappedIndices;
          remappedIndices.SetCountUninitialized(uiIndexCount);
          meshopt_remapIndexBuffer(remappedIndices.GetData(), indices.GetData(), uiIndexCount, remap.GetData());
          indices.Swap(remappedIndices);

          ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> remappedStream;

          for (ezUInt32 i = 0; i < mb.GetNumVertexBuffers(); ++i)
          {
            const auto type = static_cast<ezMeshVertexStreamType::Enum>(i);
            auto& streamData = mb.GetVertexBufferData(type);

            if (streamData.IsEmpty())
              continue;

            const ezUInt32 uiElementSize = mb.GetVertexStreamConfig().GetStreamElementSize(type);

            remappedStream.SetCountUninitialized(streamData.GetCount());
            meshopt_remapVertexBuffer(remappedStream.GetData(), streamData.GetData(), uiVertexCount, uiElementSize, remap.GetData());
            streamData = remappedStream;
          }
        }
        else
        {
          ezLog::Dev("Mesh has {} unused vertices, skipping vertex fetch optimization.", uiVertexCount - uiUniqueVertices);
        }
      }

      // write the indices back
      {
        auto& indexData = mb.GetIndexBufferData();

        if (bIndices32Bit)
        {
          ezMemoryUtils::Copy(reinterpret_cast<ezUInt32*>(indexData.GetData()), indices.GetData(), uiIndexCount);
        }
        else
        {
          ezUInt16* pDst = reinterpret_cast<ezUInt16*>(indexData.GetData());

          for (ezUInt32 i = 0; i < uiIndexCount; ++i)
          {
            pDst[i] = static_cast<ezUInt16>(indices[i]);
          }
        }
      }
    }
  } // namespace

  Importer::Importer() = default;
  Importer::~Importer() = default;

  ezResult Importer::Import(const ImportOptions& options, ezLogInterface* pLogInterface /*= nullptr*/, ezProgress* pProgress /*= nullptr*/)
  {
    ezResult res = EZ_FAILURE;

    ezLogInterface* pPrevLogSystem = ezLog::GetThreadLocalLogSystem();

    if (pLogInterface)
    {
      ezLog::SetThreadLocalLogSystem(pLogInterface);
    }

    {
      m_pProgress = pProgress;
      m_Options = options;

      EZ_LOG_BLOCK("ModelImport", m_Options.m_sSourceFile);

      res = DoImport();

      if (res.Succeeded() && m_Options.m_pMeshOutput != nullptr)
      {
        OptimizeMeshForRendering(*m_Options.m_pMeshOutput);
      }
    }


    ezLog::SetThreadLocalLogSystem(pPrevLogSystem);

    return res;
  }

  void OutputTexture::GenerateFileName(ezStringBuilder& out_sName) const
  {
    ezStringBuilder tmp("Embedded_", m_sFilename);

    ezPathUtils::MakeValidFilename(tmp.GetFileName(), '_', out_sName);
    out_sName.ChangeFileExtension(m_sFileFormatExtension);
  }

} // namespace ezModelImporter2
