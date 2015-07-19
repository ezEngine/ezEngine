#pragma once

#include <GameUtils/Basics.h>
#include <GameUtils/Components/TransformComponent.h>

class ezRotorComponent;
typedef ezComponentManagerSimple<ezRotorComponent> ezRotorComponentManager;

class EZ_GAMEUTILS_DLL ezRotorComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRotorComponent, ezRotorComponentManager);

public:
  ezRotorComponent();

  void Update();

  // ************************************* PROPERTIES ***********************************

public:
  ezInt32 m_iDegreeToRotate;
  float m_fAcceleration;
  float m_fDeceleration;
  ezEnum<ezBasisAxis> m_Axis;

private:
  ezQuat m_LastRotation;
};


