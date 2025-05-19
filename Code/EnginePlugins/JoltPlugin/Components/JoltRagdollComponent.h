#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>

class ezJoltUserData;
class ezSkeletonJoint;
class ezJoltWorldModule;
class ezJoltMaterial;
struct ezMsgRetrieveBoneState;
struct ezMsgAnimationPoseUpdated;
struct ezMsgPhysicsAddImpulse;
struct ezSkeletonResourceGeometry;

namespace JPH
{
  class Ragdoll;
  class RagdollSettings;
  class Shape;
} // namespace JPH

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

class EZ_JOLTPLUGIN_DLL ezJoltRagdollComponentManager : public ezComponentManager<class ezJoltRagdollComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltRagdollComponentManager(ezWorld* pWorld);
  ~ezJoltRagdollComponentManager();

  virtual void Initialize() override;

private:
  friend class ezJoltWorldModule;
  friend class ezJoltRagdollComponent;

  void Update(const ezWorldModule::UpdateContext& context);
};

/// \brief With which pose a ragdoll should start.
struct ezJoltRagdollStartMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    WithBindPose,        ///< The ragdoll uses the bind pose (or rest pose) to start with.
    WithNextAnimPose,    ///< The ragdoll waits for a new pose and then starts simulating with that.
    WithCurrentMeshPose, ///< The ragdoll retrieves the current pose from the animated mesh and starts simulating with that.
    Default = WithBindPose
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltRagdollStartMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Creates a physics ragdoll for an animated mesh and creates animation poses from the physics simulation.
///
/// By activating this component on an animated mesh, the component creates the necessary physics shapes to simulate a falling body.
/// The component queries the bone transforms from the physics engine and sends ezMsgAnimationPoseUpdated with new poses.
///
/// Once this component is active on an animated mesh, no other pose generating component should be active anymore, otherwise
/// multiple components generate conflicting animation poses.
/// The typical way to use this, is to have the component created and configured, but in an inactive state. Once an NPC dies, the component is activated and all other components that generate animation poses should be deactivated.
///
/// The ragdoll shapes are configured through the ezSkeletonResource.
///
/// Ragdolls are also used to create fake "breakable" objects. This is achieved by building a skinned object out of several pieces
/// and giving every piece a bone that has no constraint (joint), so that the object just breaks apart.
class EZ_JOLTPLUGIN_DLL ezJoltRagdollComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltRagdollComponent, ezComponent, ezJoltRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltRagdollComponent

public:
  ezJoltRagdollComponent();
  ~ezJoltRagdollComponent();

  /// \brief Returns the object ID used for all bodies in the ragdoll. This can be used to ignore the entire ragdoll during raycasts and such.
  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  /// \brief Adjusts how strongly gravity affects the ragdoll.
  ///
  /// Set this to zero to fully disable gravity.
  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  /// \brief If true, the ragdoll pieces collide with each other.
  /// This produces more realistic ragdoll behavior, but requires that the ragdoll shapes are well set up.
  /// It may produce jittering and also cost more performance.
  /// Another option is to set up the joint limits such that the ragdoll can't easily intersect itself.
  bool m_bSelfCollision = false; // [ property ]

  /// \brief How easily the joints move. Note that scaling the ragdoll up or down affects the forces and thus stiffness needs to be adjusted as well.
  float m_fStiffnessFactor = 1.0f; // [ property ]

  ezUInt8 m_uiWeightCategory = 0;  // [ property ]
  float m_fWeightValue = 1.0f;     // [ property ]

  /// \brief Sets with which pose the ragdoll should start simulating.
  void SetStartMode(ezEnum<ezJoltRagdollStartMode> mode);                     // [ property ]
  ezEnum<ezJoltRagdollStartMode> GetStartMode() const { return m_StartMode; } // [ property ]

  /// \brief Applies a force to a specific part of the ragdoll.
  void OnMsgPhysicsAddImpulse(ezMsgPhysicsAddImpulse& ref_msg); // [ msg handler ]

  /// \brief Call this function BEFORE activating the ragdoll component to specify an impulse that shall be applied to the closest body part when it activates.
  ///
  /// Both position and direction are given in world space.
  ///
  /// This overrides any previously set or accumulated impulses.
  /// If AFTER this call additional impulses are recorded through OnMsgPhysicsAddImpulse(), they are 'added' to the initial impulse.
  ///
  /// Only a single initial impulse is applied after the ragdoll is created.
  /// If multiple impulses are added through OnMsgPhysicsAddImpulse(), their average start position is used to determine the closest body part to apply the impulse on.
  /// Their impulses are accumulated, so the applied impulse can become quite large.
  void SetInitialImpulse(const ezVec3& vPosition, const ezVec3& vDirectionAndStrength); // [ scriptable ]

  /// \brief Adds to the existing initial impulse. See SetInitialImpulse().
  void AddInitialImpulse(const ezVec3& vPosition, const ezVec3& vDirectionAndStrength); // [ scriptable ]

  /// \brief How much of the owner object's velocity to transfer to the new ragdoll bodies.
  float m_fOwnerVelocityScale = 1.0f; // [ property ]

  /// If non-zero, when the ragdoll starts, all pieces start out with an outward velocity with this speed.
  ///
  /// This can be used to create breakable objects that should "break apart". Once the ragdoll starts, the bodies will fly
  /// away outward from their center (plus m_vCenterPosition).
  float m_fCenterVelocity = 0.0f; // [ property ]

  /// Similar to m_fCenterVelocity but sets a rotational velocity, so that objects also spin.
  float m_fCenterAngularVelocity = 0.0f; // [ property ]

  /// If center velocity is used, this adds an offset to the object's position to define where the center position should be.
  ezVec3 m_vCenterPosition = ezVec3::MakeZero(); // [ property ]

  /// \brief Allows to override the type of joint to be used for a bone.
  ///
  /// This has to be called before the component is activated.
  /// Its intended use case is to make certain joints either stiff (by setting the joints to 'fixed'),
  /// or to break pieces off (by setting the type to 'None').
  ///
  /// For example a breakable object could be made up of 10 pieces, but when breaking it, a random number of joints
  /// can be set to 'fixed' or 'none', so that the exact shape of the broken pieces has more variety.
  ///
  /// Similarly, on a animated mesh that is specifically authored to have separable pieces (like an arm on a robot),
  /// one can separate limbs by setting their joint to 'none'.
  void SetJointTypeOverride(ezStringView sJointName, ezEnum<ezSkeletonJointType> type);

  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg); // [ msg handler ]
  void OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const; // [ msg handler ]

protected:
  ezEnum<ezJoltRagdollStartMode> m_StartMode;                      // [ property ]
  float m_fGravityFactor = 1.0f;                                   // [ property ]

  struct Limb
  {
    ezUInt16 m_uiPartIndex = ezInvalidJointIndex;
  };

  struct LimbConstructionInfo
  {
    ezTransform m_GlobalTransform;
    ezUInt16 m_uiJoltPartIndex = ezInvalidJointIndex;
  };

  void Update(bool bForce);
  ezResult EnsureSkeletonIsKnown();
  void CreateLimbsFromBindPose();
  void CreateLimbsFromCurrentMeshPose();
  void DestroyAllLimbs();
  void CreateLimbsFromPose(const ezMsgAnimationPoseUpdated& pose);
  bool HasCreatedLimbs() const;
  ezTransform GetRagdollRootTransform() const;
  void UpdateOwnerPosition();
  void RetrieveRagdollPose();
  void SendAnimationPoseMsg();
  void ConfigureRagdollPart(void* pRagdollSettingsPart, const ezTransform& globalTransform, ezUInt8 uiCollisionLayer, ezJoltWorldModule& worldModule);
  void CreateAllLimbs(const ezSkeletonResource& skeletonResource, const ezMsgAnimationPoseUpdated& pose, ezJoltWorldModule& worldModule, float fObjectScale);
  void ComputeLimbModelSpaceTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiPoseJointIndex);
  void ComputeLimbGlobalTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiPoseJointIndex);
  void CreateLimb(const ezSkeletonResource& skeletonResource, ezMap<ezUInt16, LimbConstructionInfo>& limbConstructionInfos, ezArrayPtr<const ezSkeletonResourceGeometry*> geometries, const ezMsgAnimationPoseUpdated& pose, ezJoltWorldModule& worldModule, float fObjectScale);
  JPH::Shape* CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const ezSkeletonResourceGeometry& geo, const ezJoltMaterial* pJoltMaterial, const ezQuat& qBoneDirAdjustment, const ezTransform& skeletonRootTransform, ezTransform& out_shapeTransform, float fObjectScale);
  void CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, ezArrayPtr<const ezSkeletonResourceGeometry*> geometries, const ezSkeletonJoint& thisLimbJoint, const ezSkeletonResource& skeletonResource, float fObjectScale);
  virtual void ApplyPartInitialVelocity();
  void ApplyBodyMass(float fMass);
  void ApplyInitialImpulse(ezJoltWorldModule& worldModule, float fMaxImpulse);

  float GetWeightValue() const { return m_fWeightValue; }
  void SetWeightValue_Scale(float fValue);
  void SetWeightValue_Mass(float fValue);

  ezSkeletonResourceHandle m_hSkeleton;
  ezDynamicArray<ezMat4> m_CurrentLimbTransforms;

  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
  ezUInt32 m_uiJoltUserDataIndex = ezInvalidIndex;
  ezJoltUserData* m_pJoltUserData = nullptr;

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::RagdollSettings* m_pRagdollSettings = nullptr;
  ezDynamicArray<Limb> m_Limbs;
  ezTransform m_RootBodyLocalTransform;
  ezTime m_ElapsedTimeSinceUpdate = ezTime::MakeZero();

  ezVec3 m_vInitialImpulsePosition = ezVec3::MakeZero();
  ezVec3 m_vInitialImpulseDirection = ezVec3::MakeZero();
  ezUInt8 m_uiNumInitialImpulses = 0;

  struct JointOverride
  {
    ezTempHashedString m_sJointName;
    bool m_bOverrideType = false;
    ezEnum<ezSkeletonJointType> m_JointType;
  };

  ezDynamicArray<JointOverride> m_JointOverrides;

  //////////////////////////////////////////////////////////////////////////

  void SetupLimbJoints(const ezSkeletonResource* pSkeleton);
  void CreateLimbJoint(const ezSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc);
};
