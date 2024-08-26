#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;

namespace physx
{
  class PxArticulationLink;
  class PxArticulation;
  class PxAggregate;
  class PxSphericalJoint;
  struct PxFilterData;
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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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

  void SetSurfaceFile(const char* szFile);                     // [ property ]
  const char* GetSurfaceFile() const;                          // [ property ]

  ezUInt8 m_uiCollisionLayer = 0;                              // [ property ]
  ezUInt16 m_uiPieces = 16;                                    // [ property ]
  float m_fThickness = 0.05f;                                  // [ property ]
  float m_fSlack = 0.3f;                                       // [ property ]
  bool m_bAttachToOrigin = true;                               // [ property ]
  bool m_bAttachToAnchor = true;                               // [ property ]
  ezAngle m_MaxBend = ezAngle::MakeFromDegree(30);             // [ property ]
  ezAngle m_MaxTwist = ezAngle::MakeFromDegree(15);            // [ property ]

  void SetAnchorReference(const char* szReference);            // [ property ]
  void SetAnchor(ezGameObjectHandle hActor);

  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg);
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg);

private:
  void CreateRope();
  ezResult CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const;
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  void CreateFilterData(physx::PxFilterData& filter);
  physx::PxMaterial* GetPxMaterial();
  physx::PxJoint* CreateJoint(const ezGameObjectHandle& hTarget, const ezTransform& location, physx::PxRigidBody* pLink, const ezTransform& linkOffset);
  void UpdatePreview();

  ezSurfaceResourceHandle m_hSurface;

  ezGameObjectHandle m_hAnchor;

  float m_fTotalMass = 1.0f;
  float m_fMaxForcePerFrame = 0.0f;
  float m_fBendStiffness = 0.0f;
  float m_fBendDamping = 50.0f;
  float m_fTwistStiffness = 0.0f;
  float m_fTwistDamping = 50.0f;
  ezUInt32 m_uiShapeID = ezInvalidIndex;
  bool m_bSelfCollision = false;
  bool m_bDisableGravity = false;
  ezVec3 m_vPreviewRefPos = ezVec3::MakeZero();

  physx::PxAggregate* m_pAggregate = nullptr;
  physx::PxArticulation* m_pArticulation = nullptr;
  physx::PxJoint* m_pJointOrigin = nullptr;
  physx::PxJoint* m_pJointAnchor = nullptr;
  ezDynamicArray<physx::PxArticulationLink*> m_ArticulationLinks;

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;

private:
  const char* DummyGetter() const { return nullptr; }
};
