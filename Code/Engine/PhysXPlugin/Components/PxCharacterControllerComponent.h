#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

struct ezInputComponentMessage;

typedef ezComponentManagerSimple<class ezPxCharacterControllerComponent, true> ezPxCharacterControllerComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCharacterControllerComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterControllerComponent, ezPhysXComponent, ezPxCharacterControllerComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxCharacterControllerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

  virtual ezComponent::Initialization Initialize() override;

  virtual void Deinitialize() override;

  // ************************************* PROPERTIES ***********************************
public:


protected:


  // ************************************* FUNCTIONS *****************************

public:

  void InputComponentMessageHandler(ezInputComponentMessage& msg);

private:
  PxCapsuleController* m_pController;
  ezVec3 m_vRelativeMoveDirection;
  ezAngle m_RotateZ;
};


