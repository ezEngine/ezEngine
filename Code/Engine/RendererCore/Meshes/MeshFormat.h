#pragma once

#include <Foundation/IO/Stream.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

class EZ_RENDERERCORE_DLL ezMeshFormatBuilder
{
public:

  ezMeshBufferResourceDescriptor& MeshBufferDesc();

  void AddSubMesh(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiMaterialIndex);

  void SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void WriteMeshFormat(ezStreamWriterBase& stream);

  ezResult WriteMeshFormat(const char* szFile);

private:

  struct SubMesh
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiPrimitiveCount;
    ezUInt32 m_uiFirstPrimitive;
    ezUInt32 m_uiMaterialIndex;
  };

  struct Material
  {
    ezString m_sPath;
  };

  ezHybridArray<Material, 32> m_Materials;
  ezHybridArray<SubMesh, 32> m_SubMeshes;
  ezMeshBufferResourceDescriptor m_MeshBufferDescriptor;
};

