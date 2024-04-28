#pragma once

#include <Core/World/World.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

class EZ_GAMEENGINE_DLL ezLodAnimatedMeshComponentManager : public ezComponentManager<class ezLodAnimatedMeshComponent, ezBlockStorageType::FreeList>
{
public:
  ezLodAnimatedMeshComponentManager(ezWorld* pWorld);
  ~ezLodAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
  void AddToUpdateList(ezLodAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezDeque<ezComponentHandle> m_ComponentsToUpdate;
};

struct ezLodAnimatedMeshLod
{
  const char* GetMeshFile() const;
  void SetMeshFile(const char* szFile);

  ezMeshResourceHandle m_hMesh;
  float m_fThreshold;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezLodAnimatedMeshLod);

class EZ_GAMEENGINE_DLL ezLodAnimatedMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLodAnimatedMeshComponent, ezRenderComponent, ezLodAnimatedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezMeshRenderData* CreateRenderData() const;
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLodAnimatedMeshComponent

public:
  ezLodAnimatedMeshComponent();
  ~ezLodAnimatedMeshComponent();

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const ezColor& color); // [ property ]
  const ezColor& GetColor() const;     // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;       // [ property ]

  /// \brief Enables text output to show the current coverage value and selected LOD.
  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// \brief Disabling the LOD range overlap functionality can make it easier to determine the desired coverage thresholds.
  void SetOverlapRanges(bool bOverlap);                 // [ property ]
  bool GetOverlapRanges() const;                        // [ property ]

  void OnMsgSetColor(ezMsgSetColor& ref_msg);           // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg); // [ msg handler ]

  void RetrievePose(ezDynamicArray<ezMat4>& out_modelTransforms, ezTransform& out_rootTransform, const ezSkeleton& skeleton);

protected:
  void UpdateSelectedLod(const ezView& view) const;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  mutable ezInt32 m_iCurLod = 0;
  ezDynamicArray<ezLodAnimatedMeshLod> m_Meshes;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4::MakeZero();
  float m_fSortingDepthOffset = 0.0f;
  ezVec3 m_vBoundsOffset = ezVec3::MakeZero();
  float m_fBoundsRadius = 1.0f;

protected:
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones, const ezSkeleton& skeleton, ezArrayPtr<const ezMat4> modelSpaceTransforms, ezBoundingBox* bounds);

  ezTransform m_RootTransform = ezTransform::MakeIdentity();
  ezBoundingBox m_MaxBounds;
  ezSkinningState m_SkinningState;
  ezSkeletonResourceHandle m_hDefaultSkeleton;
};
