#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

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

  void Clear();

  ezMeshBufferResourceDescriptor& MeshBufferDesc();

  const ezMeshBufferResourceDescriptor& MeshBufferDesc() const;

  void UseExistingMeshBuffer(const ezMeshBufferResourceHandle& hBuffer);

  void AddSubMesh(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiMaterialIndex);

  void SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void Save(ezStreamWriter& stream);
  ezResult Save(const char* szFile);

  ezResult Load(ezStreamReader& stream);
  ezResult Load(const char* szFile);

  const ezMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  ezArrayPtr<const Material> GetMaterials() const;

  ezArrayPtr<const SubMesh> GetSubMeshes() const;

  void ComputeBounds();
  const ezBoundingBoxSphere& GetBounds() const;

  void SetSkeleton(const ezSkeletonResourceHandle& hSkeleton);
  const ezSkeletonResourceHandle& GetSkeleton() const;

private:

  ezHybridArray<Material, 8> m_Materials;
  ezHybridArray<SubMesh, 8> m_SubMeshes;
  ezMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  ezMeshBufferResourceHandle m_hMeshBuffer;
  ezSkeletonResourceHandle m_hSkeleton;

  ezBoundingBoxSphere m_Bounds;
};

