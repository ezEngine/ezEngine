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

  void SetAttachToReference(const char* szReference); // [ property ]

  ezGameObjectHandle m_hAttachTo;

protected:
  void Update();
  void ReleaseGrabbedObject();

  ezTime m_AboveBreakdistanceSince;
  float m_fPrevMass = 0.0f;
  bool m_bPrevGravity = true;
  ezUInt32 m_uiPrevPosIterations;
  ezUInt32 m_uiPrevVelIterations;
  ezComponentHandle m_hJoint;
  ezComponentHandle m_hGrabbedActor;
  ezTransform m_ChildAnchorLocal;

private:
  const char* DummyGetter() const { return nullptr; }
};
