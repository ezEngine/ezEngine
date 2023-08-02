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
struct ezMsgPhysicsAddForce;
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

struct ezJoltRagdollStartMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    WithBindPose,
    WithNextAnimPose,
    WithCurrentMeshPose,
    Default = WithBindPose
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltRagdollStartMode);

//////////////////////////////////////////////////////////////////////////

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

  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg);      // [ msg handler ]
  void OnMsgAnimationPoseProposal(ezMsgAnimationPoseProposal& ref_msg); // [ msg handler ]
  void OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const;      // [ msg handler ]

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void SetGravityFactor(float fFactor);                       // [ property ]

  bool m_bSelfCollision = false;   // [ property ]
  float m_fStiffnessFactor = 1.0f; // [ property ]
  float m_fMass = 50.0f;           // [ property ]

  void SetStartMode(ezEnum<ezJoltRagdollStartMode> mode);                     // [ property ]
  ezEnum<ezJoltRagdollStartMode> GetStartMode() const { return m_StartMode; } // [ property ]

  void OnMsgPhysicsAddImpulse(ezMsgPhysicsAddImpulse& ref_msg); // [ msg handler ]

  /// \brief Applies an impulse to a specific part of the ragdoll.
  ///
  /// If this is called before the ragdoll becomes active, it is added to the 'initial impulse' (see SetInitialImpulse()).
  /// Once the ragdoll is activated, this initial impulse is applied to the closest body part.
  void OnMsgPhysicsAddForce(ezMsgPhysicsAddForce& ref_msg); // [ msg handler ]

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
  float m_fOwnerVelocityScale = 1.0f;              // [ property ]
  float m_fCenterVelocity = 0.0f;                  // [ property ]
  float m_fCenterAngularVelocity = 0.0f;           // [ property ]
  ezVec3 m_vCenterPosition = ezVec3::MakeZero(); // [ property ]

  void SetJointTypeOverride(ezStringView sJointName, ezEnum<ezSkeletonJointType> type);

protected:
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
  void ApplyBodyMass();
  void ApplyInitialImpulse(ezJoltWorldModule& worldModule, float fMaxImpulse);

  ezEnum<ezJoltRagdollStartMode> m_StartMode; // [ property ]
  float m_fGravityFactor = 1.0f;              // [ property ]

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
