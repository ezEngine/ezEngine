#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgAnimationPoseUpdated;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

using ezJoltHitboxComponentManager = ezComponentManager<class ezJoltHitboxComponent, ezBlockStorageType::Compact>;

/// \brief Adds physics shapes to an animated character for hit detection.
///
/// Attach this component to an animated mesh, to give it a physical representation.
/// The shapes for each bone are defined through the skeleton.
///
/// Typically these shapes are "query shapes" only, meaning they don't participate in the physical simulation,
/// so they won't push other objects aside.
/// They can only be detected through raycasts and scene queries (assuming those queries have the ezPhysicsShapeType::Query flag set).
class EZ_JOLTPLUGIN_DLL ezJoltHitboxComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltHitboxComponent, ezComponent, ezJoltHitboxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltHitboxComponent

public:
  ezJoltHitboxComponent();
  ~ezJoltHitboxComponent();

  /// \brief The same object filter ID is assigned to all hit shapes.
  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  /// \brief If true, shapes can only be detected with raycasts and scene queries. If false, they will be kinematic objects in the simulation and push other rigid bodies aside.
  bool m_bQueryShapeOnly = true; // [ property ]

  /// \brief At which interval to update the hitbox transforms. Set to zero for full updates every frame.
  ezTime m_UpdateThreshold; // [ property ]

  /// \brief Updates the shape transforms to conform with the new pose, but only if the update threshold was exceeded.
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
