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

  bool GrabNearbyObject();
  bool HasObjectGrabbed() const;
  void DropGrabbedObject();
  void ThrowGrabbedObject(/*const ezVec3& vRelativeDir*/);
  void BreakObjectGrab();

  float m_fBreakDistance = 1.0f;
  float m_fSpringStiffness = 50.0f;
  float m_fSpringDamping = 10.0f;

protected:
  void Update();
  void ReleaseGrabbedObject();

  ezTime m_AboveBreakdistanceSince;
  float m_fPrevMass = 0.0f;
  ezUInt32 m_uiPrevPosIterations;
  ezUInt32 m_uiPrevVelIterations;
  ezComponentHandle m_hJoint;
  ezComponentHandle m_hGrabbedActor;
};
