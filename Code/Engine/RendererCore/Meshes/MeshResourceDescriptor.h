#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

class EZ_RENDERERCORE_DLL ezMeshResourceDescriptor
{
public:

  struct SubMesh
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiPrimitiveCount;
    ezUInt32 m_uiFirstPrimitive;
    ezUInt32 m_uiMaterialIndex;

    ezBoundingBoxSphere m_Bounds;
  };

  struct Material
  {
    ezString m_sPath;
  };

  ezMeshResourceDescriptor();

  ezMeshBufferResourceDescriptor& MeshBufferDesc();

  const ezMeshBufferResourceDescriptor& MeshBufferDesc() const;

  void UseExistingMeshBuffer(const ezMeshBufferResourceHandle& hBuffer);

  void AddSubMesh(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiMaterialIndex);

  void SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void Save(ezStreamWriterBase& stream);
  ezResult Save(const char* szFile);

  ezResult Load(ezStreamReaderBase& stream);
  ezResult Load(const char* szFile);

  const ezMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  const ezHybridArray<Material, 32>& GetMaterials() const;

  const ezHybridArray<SubMesh, 32>& GetSubMeshes() const;

  void CalculateBounds();
  const ezBoundingBoxSphere& GetBounds() const;

private:

  ezHybridArray<Material, 32> m_Materials;
  ezHybridArray<SubMesh, 32> m_SubMeshes;
  ezMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  ezMeshBufferResourceHandle m_hMeshBuffer;

  ezBoundingBoxSphere m_Bounds;
};

