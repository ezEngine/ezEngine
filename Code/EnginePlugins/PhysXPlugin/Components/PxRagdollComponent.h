#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Declarations.h>
#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgAnimationPoseUpdated;
struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;
struct ezSkeletonResourceGeometry;
class ezPxUserData;
class ezSkeletonJoint;
struct ezMsgAnimationPoseProposal;
class ezPhysXWorldModule;
struct ezMsgRetrieveBoneState;

namespace physx
{
  class PxPhysics;
  class PxAggregate;
  class PxArticulation;
  class PxArticulationDriveCache;
  class PxRigidActor;
  class PxMaterial;
  struct PxFilterData;
} // namespace physx

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

// using ezPxRagdollComponentManager = ezComponentManagerSimple<class ezPxRagdollComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

struct ezPxRagdollStart
{
  using StorageType = ezUInt8;

  enum Enum
  {
    BindPose,
    WaitForPose,
    Wait,
    Default = BindPose
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxRagdollStart);

//////////////////////////////////////////////////////////////////////////

struct EZ_PHYSXPLUGIN_DLL ezPxRagdollConstraint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPxRagdollConstraint, ezReflectedClass);

  ezString m_sBone;
  ezVec3 m_vRelativePosition;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxRagdollComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxRagdollComponent, ezPxComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRagdollComponent

public:
  ezPxRagdollComponent();
  ~ezPxRagdollComponent();

  ezUInt32 GetShapeId() const { return m_uiPxShapeID; }              // [ scriptable ]

  void OnAnimationPoseProposal(ezMsgAnimationPoseProposal& ref_msg); // [ msg handler ]
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg);   // [ msg handler ]
  void OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const;   // [ msg handler ]

  bool GetDisableGravity() const { return m_bDisableGravity; }       // [ property ]
  void SetDisableGravity(bool b);                                    // [ property ]

  bool m_bSelfCollision = false;                                     // [ property ]

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg);             // [ message ]
  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg);                 // [ message ]

protected:
  struct Limb
  {
    ezHashedString m_sName;
    physx::PxRigidBody* m_pPxBody = nullptr;
  };

  struct LimbConfig
  {
    ezTransform m_GlobalTransform;
    physx::PxRigidBody* m_pPxBody = nullptr;
  };

  struct Impulse
  {
    ezVec3 m_vPos = ezVec3::MakeZero();
    ezVec3 m_vImpulse = ezVec3::MakeZero();
    physx::PxRigidBody* m_pRigidBody = nullptr;
  };

  Impulse m_NextImpulse;
  ezDynamicArray<ezPxRagdollConstraint> m_Constraints; // [ property ]
  ezEnum<ezPxRagdollStart> m_Start;                    // [ property ]
  ezSkeletonResourceHandle m_hSkeleton;

  bool m_bDisableGravity = false;                      // [ property ]
  bool m_bLimbsSetup = false;
  ezUInt32 m_uiPxShapeID = ezInvalidIndex;
  ezUInt32 m_uiPxUserDataIndex = ezInvalidIndex;
  ezPxUserData* m_pPxUserData = nullptr;
  ezDynamicArray<Limb> m_Limbs;
  physx::PxAggregate* m_pPxAggregate = nullptr;
  physx::PxRigidBody* m_pPxRootBody = nullptr;
  ezTransform m_RootBodyLocalTransform;
  ezDynamicArray<ezMat4> m_LimbPoses;

  void Update();
  void CreateConstraints();
  void SetupLimbsFromBindPose();
  bool EnsureSkeletonIsKnown();
  virtual void ClearPhysicsObjects();
  virtual void SetupPxBasics(physx::PxPhysics* pPxApi, ezPhysXWorldModule* pPxModule);
  virtual void FinishSetupLimbs() {}
  void SetupLimbs(const ezMsgAnimationPoseUpdated& pose);
  void SetupLimbBodiesAndGeometry(const ezSkeletonResource* pSkeleton, const ezMsgAnimationPoseUpdated& pose);
  void SetupLimbJoints(const ezSkeletonResource* pSkeleton);
  virtual void CreateLimbBody(physx::PxPhysics* pPxApi, const LimbConfig& parentLimb, LimbConfig& thisLimb) = 0;
  void AddLimbGeometry(ezBasisAxis::Enum srcBoneDir, physx::PxRigidActor& actor, const ezSkeletonResourceGeometry& geo);
  virtual void CreateLimbJoint(physx::PxPhysics* pPxApi, const ezSkeletonJoint& thisJoint, physx::PxRigidBody* pPxParentBody, const ezTransform& parentFrame, physx::PxRigidBody* pPxThisBody, const ezTransform& thisFrame) = 0;
  void ApplyImpulse();
  void ComputeLimbModelSpaceTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex);
  void ComputeLimbGlobalTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex);
  void RetrievePhysicsPose();
  virtual void WakeUp() = 0;
  virtual bool IsSleeping() const = 0;
};
