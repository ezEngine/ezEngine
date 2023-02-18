#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgAnimationPoseUpdated;
struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;
struct ezSkeletonResourceGeometry;
class ezJoltUserData;
class ezSkeletonJoint;
struct ezMsgAnimationPoseProposal;
struct ezMsgRetrieveBoneState;
class ezJoltWorldModule;
namespace JPH
{
  class RagdollSettings;
}

namespace JPH
{
  class Ragdoll;
}

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

using ezJoltRagdollComponentManager = ezComponentManagerSimple<class ezJoltRagdollComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

struct ezJoltRagdollStart
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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltRagdollStart);

//////////////////////////////////////////////////////////////////////////

struct EZ_JOLTPLUGIN_DLL ezJoltRagdollConstraint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltRagdollConstraint, ezReflectedClass);

  ezString m_sBone;
  ezVec3 m_vRelativePosition;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

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

  void OnAnimationPoseProposal(ezMsgAnimationPoseProposal& ref_msg); // [ msg handler ]
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg);   // [ msg handler ]
  void OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const;   // [ msg handler ]

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void SetGravityFactor(float fFactor);                       // [ property ]

  ezUInt8 m_uiCollisionLayer = 0; // [ property ]
  bool m_bSelfCollision = false; // [ property ]

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg); // [ message ]
  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg);     // [ message ]

protected:
  struct Limb
  {
    ezHashedString m_sName;
    // physx::PxRigidBody* m_pPxBody = nullptr;
    void* m_pBodyDesc = nullptr;
    ezUInt16 m_uiPartIndex = 0xFFFFu;
  };

  struct LimbConfig
  {
    ezTransform m_GlobalTransform;
    void* m_pBodyDesc = nullptr;
    ezUInt16 m_uiPartIndex = 0xFFFFu;
  };

  struct Impulse
  {
    ezVec3 m_vPos = ezVec3::ZeroVector();
    ezVec3 m_vImpulse = ezVec3::ZeroVector();
    // physx::PxRigidBody* m_pRigidBody = nullptr;
  };

  float m_fGravityFactor = 1.0f;                         // [ property ]
  ezDynamicArray<ezJoltRagdollConstraint> m_Constraints; // [ property ]
  ezEnum<ezJoltRagdollStart> m_Start;                    // [ property ]

  Impulse m_NextImpulse;
  ezSkeletonResourceHandle m_hSkeleton;

  bool m_bLimbsSetup = false;
  float m_fStiffness = 10.0f;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
  ezUInt32 m_uiJoltUserDataIndex = ezInvalidIndex;
  ezJoltUserData* m_pJoltUserData = nullptr;
  ezDynamicArray<Limb> m_Limbs;
  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::RagdollSettings* m_pRagdollSettings = nullptr;
  ezTransform m_RootBodyLocalTransform;
  ezDynamicArray<ezMat4> m_LimbPoses;

  void Update();
  void CreateConstraints();
  void SetupLimbsFromBindPose();
  bool EnsureSkeletonIsKnown();
  virtual void ClearPhysicsObjects();
  virtual void SetupJoltBasics(ezJoltWorldModule* pPxModule);
  virtual void FinishSetupLimbs();
  void SetupLimbs(const ezMsgAnimationPoseUpdated& pose);
  void SetupLimbBodiesAndGeometry(const ezSkeletonResource* pSkeleton, const ezMsgAnimationPoseUpdated& pose);
  void SetupLimbJoints(const ezSkeletonResource* pSkeleton);
  virtual void CreateLimbBody(const LimbConfig& parentLimb, LimbConfig& thisLimb);
  void AddLimbGeometry(ezBasisAxis::Enum srcBoneDir, LimbConfig& limb, const ezSkeletonResourceGeometry& geo);
  virtual void CreateLimbJoint(const ezSkeletonJoint& thisJoint, void* pParentBodyDesc, const ezTransform& parentFrame, void* pThisBodyDesc, const ezTransform& thisFrame);
  void ApplyImpulse();
  void ComputeLimbModelSpaceTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex);
  void ComputeLimbGlobalTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex);
  void RetrievePhysicsPose();
  virtual void WakeUp();
  virtual bool IsSleeping() const;
};
