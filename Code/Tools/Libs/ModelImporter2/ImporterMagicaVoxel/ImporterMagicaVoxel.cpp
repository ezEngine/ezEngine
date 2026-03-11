#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <ModelImporter2/ImporterMagicaVoxel/ImporterMagicaVoxel.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>


#define OGT_VOX_IMPLEMENTATION
#include <ModelImporter2/ImporterMagicaVoxel/ogt_vox.h>

#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ModelImporter2/ImporterMagicaVoxel/ogt_voxel_meshify.h>


namespace ezModelImporter2
{
  ImporterMagicaVoxel::ImporterMagicaVoxel() = default;
  ImporterMagicaVoxel::~ImporterMagicaVoxel() = default;

  ezResult ImporterMagicaVoxel::DoImport()
  {
    const char* szFileName = m_Options.m_sSourceFile;

    ezDynamicArray<ezUInt8> fileContent;
    fileContent.Reserve(1024 * 1024);

    // Read the whole file into memory since we map BSP data structures directly to memory content
    {
      ezFileReader fileReader;

      if (fileReader.Open(szFileName, 1024 * 1024).Failed())
      {
        ezLog::Error("Couldn't open '{}' for voxel import.", szFileName);
        return EZ_FAILURE;
      }

      ezUInt8 Temp[1024 * 4];

      while (ezUInt64 uiRead = fileReader.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp)))
      {
        fileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));
      }
    }

    if (fileContent.IsEmpty())
    {
      return EZ_FAILURE;
    }

    const ogt_vox_scene* scene = ogt_vox_read_scene(fileContent.GetData(), fileContent.GetCount());
    if (!scene)
    {
      ezLog::Error("Couldn't open '{}' for voxel import, read_scene failed.", szFileName);
      return EZ_FAILURE;
    }

    EZ_SCOPE_EXIT(ogt_vox_destroy_scene(scene));

    // Temp storage buffers to build the mesh streams out of
    ezDynamicArray<ezVec3> positions;
    positions.Reserve(4096);

    ezDynamicArray<ezVec3> normals;
    normals.Reserve(4096);

    ezDynamicArray<ezColorGammaUB> colors;
    colors.Reserve(4096);

    ezDynamicArray<ezUInt32> indices;
    indices.Reserve(8192);

    ezUInt32 uiIndexOffset = 0;

    for (uint32_t modelIdx = 0; modelIdx < scene->num_models; ++modelIdx)
    {
      const ogt_vox_model* model = scene->models[modelIdx];

      ogt_voxel_meshify_context ctx;
      memset(&ctx, 0, sizeof(ctx));

      ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_greedy(&ctx, model->voxel_data, model->size_x, model->size_y, model->size_z, (const ogt_mesh_rgba*)&scene->palette.color[0]);
      EZ_SCOPE_EXIT(ogt_mesh_destroy(&ctx, mesh));

      if (!mesh)
      {
        ezLog::Error("Couldn't generate mesh for voxels in file '{}'.", szFileName);
        return EZ_FAILURE;
      }

      ogt_mesh_remove_duplicate_vertices(&ctx, mesh);

      // offset mesh vertices so that the center of the mesh (center of the voxel grid) is at (0,0,0)
      // also apply the root transform in the same go
      {
        const ezVec3 originOffset = ezVec3(-(float)(model->size_x >> 1), (float)(model->size_z >> 1), (float)(model->size_y >> 1));

        for (uint32_t i = 0; i < mesh->vertex_count; ++i)
        {
          ezVec3 pos = ezVec3(-mesh->vertices[i].pos.x, mesh->vertices[i].pos.z, mesh->vertices[i].pos.y);
          pos -= originOffset;
          positions.ExpandAndGetRef() = m_Options.m_RootTransform * pos;

          ezVec3 norm = ezVec3(-mesh->vertices[i].normal.x, mesh->vertices[i].normal.z, mesh->vertices[i].normal.y);
          normals.ExpandAndGetRef() = m_Options.m_RootTransform.TransformDirection(norm);

          colors.ExpandAndGetRef() = ezColorGammaUB(mesh->vertices[i].color.r, mesh->vertices[i].color.g, mesh->vertices[i].color.b, mesh->vertices[i].color.a);
        }

        for (uint32_t i = 0; i < mesh->index_count; ++i)
        {
          indices.PushBack(mesh->indices[i] + uiIndexOffset);
        }
      }

      uiIndexOffset += mesh->vertex_count;
    }


    ezMeshBufferResourceDescriptor& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    mb.AddCommonStreams();
    mb.AddStream(ezMeshVertexStreamType::Color0);

    mb.AllocateStreams(positions.GetCount(), ezGALPrimitiveTopology::Triangles, indices.GetCount() / 3, true);

    // Add triangles
    ezUInt32 uiFinalTriIdx = 0;
    for (ezUInt32 i = 0; i < indices.GetCount(); i += 3, ++uiFinalTriIdx)
    {
      mb.SetTriangleIndices(uiFinalTriIdx, indices[i + 1], indices[i + 0], indices[i + 2]);
    }

    for (ezUInt32 i = 0; i < positions.GetCount(); ++i)
    {
      mb.SetPosition(i, positions[i]);
      mb.SetNormal(i, normals[i]);
      mb.SetColor0(i, colors[i]);
    }

    m_Options.m_pMeshOutput->SetMaterial(0, ezMaterialResource::GetDefaultMaterialFileName(ezMaterialResource::DefaultMaterialType::Lit));
    m_Options.m_pMeshOutput->AddSubMesh(indices.GetCount() / 3, 0, 0);
    m_Options.m_pMeshOutput->ComputeBounds();

    return EZ_SUCCESS;
  }
} // namespace ezModelImporter2
