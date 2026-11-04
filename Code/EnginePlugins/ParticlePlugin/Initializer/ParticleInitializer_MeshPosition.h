#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

/// Initializer that spawns particles on a mesh
///
/// Spawn positions are distributed uniformly over the mesh surface, independent of how densely the mesh is tessellated.
/// Optionally sets the initial velocity along the interpolated vertex normal at the spawn position.
///
/// If 'UseOwnerMesh' is enabled, the closest mesh on the object that the effect is attached to or one of its parents is used.
/// See ezMsgQueryMesh. Otherwise the 'Mesh' property is used, which is also what the effect editor preview shows.
/// Without any mesh, particles spawn at the origin of the effect.
///
/// The owner mesh is searched once, in the first UpdateFromWorld(), its transform relative to the effect is updated every frame.
/// For animated (skinned) owner meshes, the particle component forwards the pose (ezMsgAnimationPoseUpdated), which is used
/// with one frame of delay. Particles then spawn on the animated surface.
/// If no pose message arrived yet, the pose that the mesh is currently rendered with is used, which delays the first particles by one frame.
/// Only a mesh that has no pose at all at that point (e.g. in the very first frame of a scene) uses the rest pose.
///
/// The mesh is loaded asynchronously. Until it is available, the particle system does not emit any particles.
///
/// With 'ReferenceArea', the spawn counts of the emitters are scaled by the surface area of the mesh (including the scale of the
/// mesh object), so that larger meshes get proportionally more particles. The area is determined once, when the mesh is available.
/// If it changes the max particle count of the system, the system is recreated at that point, which costs one more frame of delay.
/// Later changes of the scale or of the mesh are ignored.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_MeshPosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_MeshPosition, ezParticleInitializer);

public:
  ezParticleInitializer_MeshPosition();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual bool GetUpdateFromWorld() const override { return m_bUseOwnerMesh; }
  virtual void UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const override;
  virtual bool GetReceiveOwnerMessages() const override { return m_bUseOwnerMesh; }
  virtual void OnOwnerMessage(const ezParticleModuleContext& ctx, const ezMessage& msg) const override;
  virtual bool PrepareToSpawn(const ezParticleModuleContext& ctx) const override;
  virtual float GetInstanceSpawnCountMultiplier(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

public:
  bool m_bUseOwnerMesh = true;   ///< If true, prefer the mesh of the object that the effect is attached to over m_sMesh
  ezString m_sMesh;              ///< The mesh asset whose surface is used as the spawn area
  bool m_bSetVelocity = false;   ///< If true, set velocity along the surface normal
  ezVarianceTypeFloat m_Speed;   ///< Speed value when setting velocity
  float m_fReferenceArea = 0.0f; ///< If larger than zero, the emitter spawn counts apply to a mesh with this surface area (in square meters) and are scaled by the actual area.

private:
  ezCpuMeshResourceHandle m_hMesh;

private:
  struct Triangle
  {
    ezVec3 m_vPosition[3];
    ezVec3 m_vNormal[3];
  };

  struct TriangleSkinning
  {
    ezVec4U16 m_vBoneIndices[3];
    ezVec4 m_vBoneWeights[3];
  };

  /// Computed once per mesh (and resource change) and shared through ezParticleWorldModule::GetOrCreateSharedData(). Read-only once created.
  struct SurfaceData : public ezRefCounted
  {
    ezDynamicArray<Triangle> m_Triangles;
    ezDynamicArray<float> m_AreaPrefixSum;               ///< m_AreaPrefixSum[i] is the summed area of triangles 0 to i (inclusive).
    ezDynamicArray<TriangleSkinning> m_TriangleSkinning; ///< Same size as m_Triangles if the mesh has skinning data, otherwise empty.
  };

  struct PoseBone
  {
    ezMat4 m_mInverseRestPose;
    ezUInt16 m_uiBoneIndex;
    ezUInt16 m_uiJointIndex;
  };

  struct InstanceData
  {
    bool HasSurface() const { return m_pSurfaceData != nullptr && !m_pSurfaceData->m_Triangles.IsEmpty(); }

    /// Only the owner mesh can have an animated pose. False until PrepareToSpawn() has set up the surface data.
    bool UsesSkinnedOwnerMesh() const { return m_bActiveMeshIsOwnerMesh && m_pSurfaceData != nullptr && !m_pSurfaceData->m_TriangleSkinning.IsEmpty(); }

    ezProcessingStream* m_pStreamPosition = nullptr;
    ezProcessingStream* m_pStreamVelocity = nullptr;

    ezCpuMeshResourceHandle m_hActiveMesh;                           ///< Either the mesh of the module or the owner mesh of the effect.
    bool m_bActiveMeshIsOwnerMesh = false;                           ///< Only the owner mesh can have an animated pose.
    ezTransform m_ActiveMeshTransform = ezTransform::MakeIdentity(); ///< Transforms from the space of m_hActiveMesh into the space of the effect.

    ezCpuMeshResourceHandle m_hSurfaceDataMesh;                      ///< The mesh from which m_pSurfaceData was built.
    ezUInt32 m_uiSurfaceDataChangeCounter = 0;                       ///< Resource change counter of m_hSurfaceDataMesh when m_pSurfaceData was built, to detect hot-reloading.
    bool m_bSurfaceDataValid = false;                                ///< Set once the mesh was loaded, even if it contains no usable triangles.
    ezSharedPtr<ezRefCounted> m_pSurfaceDataRef;                     ///< Keeps m_pSurfaceData alive.
    const SurfaceData* m_pSurfaceData = nullptr;                     ///< nullptr if there is no usable surface.

    /// Skinned meshes are rendered with the root transform of their skeleton (e.g. a change of the up axis or of units). Without an animated pose,
    /// the vertices have to be transformed with it as well. Identity for static meshes.
    ezTransform m_RestPoseTransform = ezTransform::MakeIdentity();
    ezSkeletonResourceHandle m_hRestPoseSkeleton; ///< Set until m_RestPoseTransform was read from it.

    // Owner mesh, written in UpdateFromWorld()
    bool m_bOwnerMeshSearched = false;
    ezGameObjectHandle m_hOwnerMeshObject;
    ezCpuMeshResourceHandle m_hOwnerMesh;
    ezTransform m_OwnerMeshTransform = ezTransform::MakeIdentity(); ///< Transforms from the space of the owner mesh object into the space of the effect.

    // Pose of the owner mesh, all in the local space of the owner mesh object
    const ezSkeleton* m_pPoseSkeleton = nullptr; ///< The skeleton for which m_PoseBones was built.
    ezUInt32 m_uiPoseMeshChangeCounter = 0;      ///< Resource change counter of the owner mesh when m_PoseBones was built.
    ezUInt16 m_uiNumPoseBones = 0;               ///< Number of skinning matrices, highest mesh bone index + 1.
    ezDynamicArray<PoseBone> m_PoseBones;        ///< Maps mesh bones to skeleton joints.
    bool m_bPosePending = false;                 ///< Set by OnOwnerMessage(), m_PendingSkinningMatrices is made current in UpdateFromWorld().
    bool m_bCurrentPoseChecked = false;          ///< Set once UpdateFromWorld() tried to get the pose that the mesh is rendered with, particles are not spawned before that.
    ezDynamicArray<ezMat4> m_PendingSkinningMatrices;
    ezDynamicArray<ezMat4> m_SkinningMatrices;   ///< Used by the particle update. Indexed by the bone indices stored in the mesh vertices.
  };

  /// Queries the owner object and then its parents up to the root for a mesh. The closest one wins.
  void FindOwnerMesh(const ezParticleModuleContext& ctx, InstanceData& data, const ezWorld& world, ezGameObjectHandle hOwnerObject) const;

  /// Computes the skinning matrices from the pose into m_PendingSkinningMatrices.
  static void UpdatePendingPose(InstanceData& data, const ezMsgAnimationPoseUpdated& msg);

  /// Takes the pose that the owner mesh is currently rendered with. Pose messages are only sent while the pose changes,
  /// e.g. not anymore once a ragdoll came to rest, so an effect that starts after that would otherwise never get a pose.
  static void RetrieveCurrentPose(InstanceData& data, const ezGameObject& meshObject);

  /// Gets the surface data of the active mesh from the world module, which shares it between all effects. Returns false while the mesh isn't loaded yet.
  static bool UpdateSurfaceData(const ezParticleModuleContext& ctx, InstanceData& data);

  /// Reads the root transform of the default skeleton of a skinned mesh. Returns false while the skeleton isn't loaded yet.
  static bool UpdateRestPoseTransform(InstanceData& data);

  /// Extracts the triangles from the CPU mesh and computes the area prefix sums.
  static ezSharedPtr<ezRefCounted> CreateSurfaceData(const ezCpuMeshResource& mesh);

  /// Without skinning matrices, the rest pose is used.
  static void SampleSurface(const InstanceData& data, ezRandom& rng, ezArrayPtr<const ezMat4> skinningMatrices, ezVec3& out_vPosition, ezVec3& out_vNormal);
};
