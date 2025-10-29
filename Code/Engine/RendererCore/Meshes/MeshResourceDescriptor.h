#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

/// Descriptor for creating mesh resources.
///
/// Defines sub-meshes with materials, references or creates mesh buffer data,
/// and stores bounding information. Used both for procedural mesh generation and
/// loading from files.
class EZ_RENDERERCORE_DLL ezMeshResourceDescriptor
{
public:
  /// Describes a sub-mesh within the mesh.
  ///
  /// Each sub-mesh references a range of primitives and a material slot.
  struct SubMesh
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiPrimitiveCount;  ///< Number of primitives in this sub-mesh.
    ezUInt32 m_uiFirstPrimitive;  ///< Index of the first primitive.
    ezUInt32 m_uiMaterialIndex;   ///< Index into the material array.

    ezBoundingBoxSphere m_Bounds; ///< Bounding volume of this sub-mesh.
  };

  /// Material slot information.
  struct Material
  {
    ezString m_sPath; ///< Path or GUID of the material resource.
  };

  ezMeshResourceDescriptor();

  void Clear();

  /// Returns the mesh buffer descriptor for creating a new mesh buffer.
  ///
  /// Use this when building mesh data procedurally. Mutually exclusive with UseExistingMeshBuffer.
  ezMeshBufferResourceDescriptor& MeshBufferDesc();

  const ezMeshBufferResourceDescriptor& MeshBufferDesc() const;

  /// Uses an existing mesh buffer instead of creating a new one.
  ///
  /// Mutually exclusive with modifying MeshBufferDesc.
  void UseExistingMeshBuffer(const ezMeshBufferResourceHandle& hBuffer);

  /// Adds a sub-mesh to the descriptor.
  void AddSubMesh(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiMaterialIndex);

  /// Sets the material path for a material slot.
  void SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void Save(ezStreamWriter& inout_stream);
  ezResult Save(const char* szFile);

  ezResult Load(ezStreamReader& inout_stream);
  ezResult Load(const char* szFile);

  const ezMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  ezArrayPtr<const Material> GetMaterials() const;

  ezArrayPtr<const SubMesh> GetSubMeshes() const;

  /// Merges all submeshes into just one.
  void CollapseSubMeshes();

  void ComputeBounds();
  const ezBoundingBoxSphere& GetBounds() const;
  void SetBounds(const ezBoundingBoxSphere& bounds) { m_Bounds = bounds; }

  /// Data for a bone used in skinned meshes.
  struct BoneData
  {
    ezMat4 m_GlobalInverseRestPoseMatrix;         ///< Transform from mesh space to bone space.
    ezUInt16 m_uiBoneIndex = ezInvalidJointIndex; ///< Index into the skeleton.

    ezResult Serialize(ezStreamWriter& inout_stream) const;
    ezResult Deserialize(ezStreamReader& inout_stream);
  };

  ezSkeletonResourceHandle m_hDefaultSkeleton;   ///< Default skeleton for skinned meshes.
  ezHashTable<ezHashedString, BoneData> m_Bones; ///< Bone data indexed by bone name.

  /// Maximum distance between any vertex and its influencing bones.
  ///
  /// Can be used for adjusting the bounding box of an animated pose.
  float m_fMaxBoneVertexOffset = 0.0f;

private:
  ezHybridArray<Material, 8> m_Materials;
  ezHybridArray<SubMesh, 8> m_SubMeshes;
  ezMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  ezMeshBufferResourceHandle m_hMeshBuffer;
  ezBoundingBoxSphere m_Bounds;
};
