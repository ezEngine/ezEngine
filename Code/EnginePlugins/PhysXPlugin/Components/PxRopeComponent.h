#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;
namespace physx
{
  class PxSphericalJoint;
}

namespace physx
{
  class PxArticulationLink;
  class PxArticulation;
  class PxAggregate;
} // namespace physx

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxRopeComponentManager : public ezComponentManager<class ezPxRopeComponent, ezBlockStorageType::Compact>
{
public:
  ezPxRopeComponentManager(ezWorld* pWorld);
  ~ezPxRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxRopeComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRopeComponent, ezPxComponent, ezPxRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRopeComponent

public:
  ezPxRopeComponent();
  ~ezPxRopeComponent();

  bool GetDisableGravity() const { return m_bDisableGravity; } // [ property ]
  void SetDisableGravity(bool b);                              // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  ezUInt8 m_uiCollisionLayer = 0;           // [ property ]
  ezUInt16 m_uiPieces = 16;                 // [ property ]
  float m_fThickness = 0.05f;               // [ property ]
  bool m_bAttachToA = true;                 // [ property ]
  bool m_bAttachToB = true;                 // [ property ]
  ezAngle m_MaxBend = ezAngle::Degree(30);  // [ property ]
  ezAngle m_MaxTwist = ezAngle::Degree(15); // [ property ]

  void SetAnchorAReference(const char* szReference); // [ property ]
  void SetAnchorBReference(const char* szReference); // [ property ]

  void SetAnchorA(ezGameObjectHandle hActor);
  void SetAnchorB(ezGameObjectHandle hActor);

  void AddForceAtPos(ezMsgPhysicsAddForce& msg);
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg);

private:
  void CreateRope();
  void CreateSegmentTransforms(const ezTransform& rootTransform, ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const;
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  PxFilterData CreateFilterData();
  PxMaterial* GetPxMaterial();
  ezVec3 GetAnchorPosition(const ezGameObjectHandle& hTarget) const;
  PxJoint* CreateJoint(const ezGameObjectHandle& hTarget, const ezTransform& location, PxRigidBody* pLink, const ezTransform& linkOffset);
  void UpdatePreview();

  mutable float m_fRopeLength = 0.0f;
  ezSurfaceResourceHandle m_hSurface;

  ezGameObjectHandle m_hAnchorA;
  ezGameObjectHandle m_hAnchorB;

  float m_fTotalMass = 1.0f;
  float m_fMaxForcePerFrame = 0.0f;
  float m_fBendStiffness = 0.0f;
  float m_fBendDamping = 50.0f;
  float m_fTwistStiffness = 0.0f;
  float m_fTwistDamping = 50.0f;
  ezUInt32 m_uiShapeID = ezInvalidIndex;
  bool m_bSelfCollision = false;
  bool m_bDisableGravity = false;
  ezVec3 m_vPreviewRefPos = ezVec3::ZeroVector();

  physx::PxAggregate* m_pAggregate = nullptr;
  physx::PxArticulation* m_pArticulation = nullptr;
  physx::PxJoint* m_pJointA = nullptr;
  physx::PxJoint* m_pJointB = nullptr;
  ezDynamicArray<physx::PxArticulationLink*> m_ArticulationLinks;

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;

private:
  const char* DummyGetter() const { return nullptr; }
};
