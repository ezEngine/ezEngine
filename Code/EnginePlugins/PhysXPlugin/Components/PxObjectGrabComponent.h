#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

using ezPxGrabObjectComponentManager = ezComponentManager<class ezPxGrabObjectComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxGrabObjectComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxGrabObjectComponent, ezPxComponent, ezPxGrabObjectComponentManager);

public:
  
  ezPxGrabObjectComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;

  virtual void DeserializeComponent(ezWorldReader& stream) override;

  bool TryGrabObject();

  void ReleaseGrabbedObject();

  float m_fPrevMass = 0.0f;
  ezUInt32 m_uiPrevPosIterations;
  ezUInt32 m_uiPrevVelIterations;
  ezComponentHandle m_hJoint;
  ezComponentHandle m_hGrabbedActor;
};
