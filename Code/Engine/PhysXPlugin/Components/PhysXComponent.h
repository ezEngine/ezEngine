#pragma once

#include <PhysXPlugin/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

//struct ezTransformComponentFlags
//{
//  typedef ezUInt16 StorageType;
//
//  enum Enum
//  {
//    None                = 0,
//    Autorun             = EZ_BIT(0),
//    AutoReturnStart     = EZ_BIT(1),
//    AutoReturnEnd       = EZ_BIT(2),
//    AutoToggleDirection = EZ_BIT(3),
//    Paused              = EZ_BIT(4),
//    AnimationReversed   = EZ_BIT(5),
//    Default             = Autorun | AutoReturnStart | AutoReturnEnd | AutoToggleDirection
//  };
//
//  struct Bits
//  {
//    StorageType Autorun : 1;
//    StorageType AutoReturnStart : 1;
//    StorageType AutoReturnEnd : 1;
//    StorageType AutoToggleDirection : 1;
//    StorageType Paused : 1;
//    StorageType AnimationReversed : 1;
//  };
//};
//
//EZ_DECLARE_FLAGS_OPERATORS(ezTransformComponentFlags);

typedef ezComponentManagerAbstract<class ezPhysXComponent> ezPhysXComponentManager;

/// \brief Base class for all PhysX components, such that they all have a common ancestor
class EZ_PHYSXPLUGIN_DLL ezPhysXComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPhysXComponent, ezComponent, ezPhysXComponentManager);
  virtual void ezPhysXComponentIsAbstract() = 0; // abstract classes are not shown in the UI, since this class has no other abstract functions so far, this is a dummy

public:
  ezPhysXComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream) override {}

  // ************************************* PROPERTIES ***********************************


protected:
  //ezBitflags<ezTransformComponentFlags> m_Flags;

  // ************************************* FUNCTIONS *****************************

public:


};
