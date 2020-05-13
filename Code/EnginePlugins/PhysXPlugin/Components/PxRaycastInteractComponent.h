#pragma once

#include <Core/World/World.h>
#include <PhysXPlugin/PhysXPluginDLL.h>

struct ezPhysicsCastResult;
typedef ezComponentManager<class ezPxRaycastInteractComponent, ezBlockStorageType::FreeList> ezPxRaycastInteractComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxRaycastInteractComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRaycastInteractComponent, ezComponent, ezPxRaycastInteractComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxRaycastInteractComponent

public:
  ezPxRaycastInteractComponent();
  ~ezPxRaycastInteractComponent();

  void ExecuteInteraction(); // [ scriptable ]

  ezUInt8 m_uiCollisionLayer = 0; // [ property ]
  float m_fMaxDistance = 1.0f;    // [ property ]
  ezString m_sUserMessage;        // [ property ]

protected:
  virtual void SendMessage(const ezPhysicsCastResult& hit);
};
