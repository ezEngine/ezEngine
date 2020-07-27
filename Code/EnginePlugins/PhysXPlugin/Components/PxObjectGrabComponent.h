#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

using ezPxGrabObjectComponentManager = ezComponentManagerSimple<class ezPxGrabObjectComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxGrabObjectComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxGrabObjectComponent, ezPxComponent, ezPxGrabObjectComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxGrabObjectComponent

public:
  ezPxGrabObjectComponent();
  ~ezPxGrabObjectComponent();

  bool GrabNearbyObject();
  bool HasObjectGrabbed() const;
  void DropGrabbedObject();
  void ThrowGrabbedObject(const ezVec3& vRelativeDir);
  void BreakObjectGrab();

  float m_fBreakDistance = 0.5f;
  float m_fSpringStiffness = 50.0f;
  float m_fSpringDamping = 10.0f;
  float m_fRaycastDistance = 5.0f;
  ezUInt8 m_uiCollisionLayer = 0;

  void SetAttachToReference(const char* szReference); // [ property ]

  ezGameObjectHandle m_hAttachTo;

protected:
  void Update();
  void ReleaseGrabbedObject();

  ezPxDynamicActorComponent* FindGrabbableActor();
  ezPxDynamicActorComponent* GetAttachToActor();
  ezResult DetermineGrabPoint(ezPxDynamicActorComponent* pActor);
  void AdjustGrabbedActor(ezPxDynamicActorComponent* pActor);
  void CreateJoint(ezPxDynamicActorComponent* pParent, ezPxDynamicActorComponent* pChild);

  ezComponentHandle m_hGrabbedActor;
  bool m_bGrabbedActorGravity = true;
  float m_fGrabbedActorMass = 0.0f;
  ezUInt32 m_uiGrabbedActorPosIterations;
  ezUInt32 m_uiGrabbedActorVelIterations;

  ezComponentHandle m_hJoint;
  ezTime m_AboveBreakdistanceSince;
  ezTransform m_ChildAnchorLocal;

private:
  const char* DummyGetter() const { return nullptr; }
};
