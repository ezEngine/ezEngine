#pragma once

#include <GameUtils/Basics.h>
#include <GameUtils/Components/TransformComponent.h>

class ezRotorComponent;
typedef ezComponentManagerSimple<ezRotorComponent> ezRotorComponentManager;

struct EZ_GAMEUTILS_DLL ezRotorComponentAxis
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    PosX,
    PosY,
    PosZ,
    NegX,
    NegY,
    NegZ,

    Default = PosY
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEUTILS_DLL, ezRotorComponentAxis);

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
  ezEnum<ezRotorComponentAxis> m_Axis;

private:
  ezQuat m_LastRotation;
};


