#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct ezTransformComponentFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    None = 0,
    Running = EZ_BIT(0),
    AutoReturnStart = EZ_BIT(1),
    AutoReturnEnd = EZ_BIT(2),
    AnimationReversed = EZ_BIT(5),
    Default = Running | AutoReturnStart | AutoReturnEnd
  };

  struct Bits
  {
    StorageType Running : 1;
    StorageType AutoReturnStart : 1;
    StorageType AutoReturnEnd : 1;
    StorageType Unused1 : 1;
    StorageType Unused2 : 1;
    StorageType AnimationReversed : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezTransformComponentFlags);

class EZ_GAMEENGINE_DLL ezTransformComponent : public ezComponent
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransformComponent, ezComponent);

public:
  ezTransformComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezTransformComponent interface

public:
  void SetDirectionForwards(bool bForwards); // [scriptable]
  void ToggleDirection();                    // [scriptable]
  bool IsDirectionForwards() const;          // [scriptable]

  bool IsRunning(void) const;     // [property]
  void SetRunning(bool bRunning); // [property]

  bool GetReverseAtStart(void) const; // [property]
  void SetReverseAtStart(bool b);     // [property]

  bool GetReverseAtEnd(void) const; // [property]
  void SetReverseAtEnd(bool b);     // [property]

  float m_fAnimationSpeed; // [property]

protected:
  ezBitflags<ezTransformComponentFlags> m_Flags;
  ezTime m_AnimationTime;
};
