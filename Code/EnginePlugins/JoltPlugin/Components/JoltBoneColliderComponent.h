#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgAnimationPoseUpdated;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

using ezJoltBoneColliderComponentManager = ezComponentManager<class ezJoltBoneColliderComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltBoneColliderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltBoneColliderComponent, ezComponent, ezJoltBoneColliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltBoneColliderComponent

public:
  ezJoltBoneColliderComponent();
  ~ezJoltBoneColliderComponent();

  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  bool m_bQueryShapeOnly = true; // [ property ]
  ezTime m_UpdateThreshold;      // [ property ]

  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg); // [ msg handler ]

  /// \brief Destroys the current shape objects and creates new ones.
  ///
  /// This can be used to update the shapes when for example the object was scaled.
  /// Be aware that all child objects that were attached to the previous physics shape objects will
  /// be deleted as well. So any 'attachments' will disappear.
  void RecreatePhysicsShapes(); // [ scriptable ]

protected:
  void CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton);
  void DestroyPhysicsShapes();

  struct Shape
  {
    ezUInt16 m_uiAttachedToBone = 0xFFFF;
    ezVec3 m_vOffsetPos;
    ezQuat m_qOffsetRot;

    ezGameObjectHandle m_hActorObject;
  };

  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
  ezTime m_LastUpdate;
  ezDynamicArray<Shape> m_Shapes;
};
