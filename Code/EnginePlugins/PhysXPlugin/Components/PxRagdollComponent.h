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

namespace physx
{
  class PxAggregate;
  class PxArticulation;
  class PxArticulationDriveCache;
  class PxRigidActor;
  class PxMaterial;
  struct PxFilterData;
} // namespace physx

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

//using ezPxRagdollComponentManager = ezComponentManagerSimple<class ezPxRagdollComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

struct ezPxRagdollStart
{
  using StorageType = ezUInt8;

  enum Enum
  {
    BindPose,
    WaitForPose,
    WaitForPoseAndVelocity, // TODO: not implemented (no difference to WaitForPose)
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
};

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxRagdollComponentManager : public ezComponentManager<class ezPxRagdollComponent, ezBlockStorageType::FreeList>
{
public:
  ezPxRagdollComponentManager(ezWorld* pWorld);
  ~ezPxRagdollComponentManager();

  virtual void Initialize() override;

private:
  friend class ezPhysXWorldModule;

  void UpdateRagdolls(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxRagdollComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRagdollComponent, ezPxComponent, ezPxRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRagdollComponent

public:
  ezPxRagdollComponent();
  ~ezPxRagdollComponent();

  ezUInt32 GetShapeId() const { return m_uiShapeID; } // [ scriptable ]

  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  bool GetDisableGravity() const { return m_bDisableGravity; } // [ property ]
  void SetDisableGravity(bool b);                              // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  ezUInt8 m_uiCollisionLayer = 0; // [ property ]
  bool m_bSelfCollision = false;  // [ property ]

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg); // [ message ]
  void AddForceAtPos(ezMsgPhysicsAddForce& msg);     // [ message ]

protected:
  struct LinkData
  {
    physx::PxArticulationLink* m_pLink = nullptr;
    ezTransform m_GlobalTransform;
  };

  struct BoneData
  {
    physx::PxArticulationLink* m_pLink = nullptr;
    ezHashedString m_sBoneName;
  };

  void CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton, ezMsgAnimationPoseUpdated& poseMsg);
  void DestroyPhysicsShapes();
  void UpdatePose();
  void Update();
  void CreateShapesFromBindPose();
  void AddArticulationToScene();
  void CreateBoneShape(const ezTransform& rootTransform, ezBasisAxis::Enum srcBoneDir, physx::PxRigidActor& actor, const ezSkeletonResourceGeometry& geo, const physx::PxMaterial& pxMaterial, const physx::PxFilterData& pxFilterData, ezPxUserData* pPxUserData);
  void CreateBoneLink(ezUInt16 uiBoneIdx, const ezSkeletonJoint& bone, ezBasisAxis::Enum srcBoneDir, ezPxUserData* pPxUserData, LinkData& thisLink, const LinkData& parentLink, ezMsgAnimationPoseUpdated& poseMsg);
  void CreateConstraints();

  physx::PxMaterial* GetPxMaterial();
  physx::PxFilterData CreateFilterData();

  ezSurfaceResourceHandle m_hSurface;

  bool m_bShapesCreated = false;
  bool m_bHasFirstState = false;
  bool m_bDisableGravity = false;
  ezUInt32 m_uiShapeID = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;

  struct Impulse
  {
    ezVec3 m_vPos;
    ezVec3 m_vImpulse;
  };

  ezHybridArray<Impulse, 8> m_Impulses;
  ezDynamicArray<ezPxRagdollConstraint> m_Constraints;

  ezEnum<ezPxRagdollStart> m_Start;
  physx::PxArticulationLink* m_pRootLink = nullptr;
  ezTransform m_RootLinkLocalTransform;
  ezDynamicArray<BoneData> m_ArticulationLinks;
  ezDynamicArray<ezMat4> m_JointPoses;

  ezSkeletonResourceHandle m_hSkeleton;
  physx::PxAggregate* m_pAggregate = nullptr;
  physx::PxArticulation* m_pArticulation = nullptr;
};
