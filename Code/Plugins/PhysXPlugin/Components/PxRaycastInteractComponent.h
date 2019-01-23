#pragma once

#include <PhysXPlugin/Basics.h>
#include <Core/World/World.h>
#include <GameEngine/Surfaces/SurfaceResource.h>

struct ezPhysicsHitResult;
typedef ezComponentManager<class ezPxRaycastInteractComponent, ezBlockStorageType::FreeList> ezPxRaycastInteractComponentManager;

struct EZ_PHYSXPLUGIN_DLL ezMsgTriggerRaycastInteractionComponent : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgTriggerRaycastInteractionComponent, ezMessage);
};

class EZ_PHYSXPLUGIN_DLL ezPxRaycastInteractComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRaycastInteractComponent, ezComponent, ezPxRaycastInteractComponentManager);

public:
  ezPxRaycastInteractComponent();
  ~ezPxRaycastInteractComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnTrigger(ezMsgTriggerRaycastInteractionComponent& msg);

  // ************************************* PROPERTIES ***********************************

  ezUInt8 m_uiCollisionLayer = 0;
  float m_fMaxDistance = 1.0f;
  ezString m_sUserMessage;

protected:
  virtual void SendMessage(const ezPhysicsHitResult& hit);


};



