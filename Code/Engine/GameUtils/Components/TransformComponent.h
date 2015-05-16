#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Foundation/Time/Time.h>

class ezRotorTransformComponent;
typedef ezComponentManagerSimple<ezRotorTransformComponent> ezRotorTransformComponentManager;

struct ezTransformComponentFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    None                = 0,
    Autorun             = EZ_BIT(0),
    AutoReturnStart     = EZ_BIT(1),
    AutoReturnEnd       = EZ_BIT(2),
    AutoToggleDirection = EZ_BIT(3),
    Paused              = EZ_BIT(4),
    AnimationReversed   = EZ_BIT(5),
    Default             = Autorun | AutoReturnStart | AutoReturnEnd | AutoToggleDirection
  };

  struct Bits
  {
    StorageType Autorun : 1;
    StorageType AutoReturnStart : 1;
    StorageType AutoReturnEnd : 1;
    StorageType AutoToggleDirection : 1;
    StorageType Paused : 1;
    StorageType AnimationReversed : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezTransformComponentFlags);

class EZ_GAMEUTILS_DLL ezTransformComponent : public ezComponent
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransformComponent);

public:
  ezTransformComponent();

  bool GetAnimatingAtStartup(void) const { return (m_Flags.IsAnySet(ezTransformComponentFlags::Autorun)); }
  void SetAnimatingAtStartup(bool b) { m_Flags.AddOrRemove(ezTransformComponentFlags::Autorun, b); }

  bool GetAutoReturnStart(void) const { return (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnStart)); }
  void SetAutoReturnStart(bool b) { m_Flags.AddOrRemove(ezTransformComponentFlags::AutoReturnStart, b); }

  bool GetAutoReturnEnd(void) const { return (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnEnd)); }
  void SetAutoReturnEnd(bool b) { m_Flags.AddOrRemove(ezTransformComponentFlags::AutoReturnEnd, b); }

  bool GetAutoToggleDirection(void) const { return (m_Flags.IsAnySet(ezTransformComponentFlags::AutoToggleDirection)); }
  void SetAutoToggleDirection(bool b) { m_Flags.AddOrRemove(ezTransformComponentFlags::AutoToggleDirection, b); }

  float m_fAnimationSpeed;
  ezTime m_AnimationTime;
  ezBitflags<ezTransformComponentFlags> m_Flags;
};


class EZ_GAMEUTILS_DLL ezRotorTransformComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRotorTransformComponent, ezRotorTransformComponentManager);

public:
  ezRotorTransformComponent();

  void Update();

  ezInt32 m_iDegreeToRotate;
  float m_fAcceleration;
  float m_fDeceleration;
};


