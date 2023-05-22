#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct ezTransformComponentFlags
{
  using StorageType = ezUInt16;

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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezTransformComponent

public:
  ezTransformComponent();
  ~ezTransformComponent();

  void SetDirectionForwards(bool bForwards); // [ scriptable ]
  void ToggleDirection();                    // [ scriptable ]
  bool IsDirectionForwards() const;          // [ scriptable ]

  bool IsRunning(void) const;     // [ property ]
  void SetRunning(bool bRunning); // [ property ]

  bool GetReverseAtStart(void) const; // [ property ]
  void SetReverseAtStart(bool b);     // [ property ]

  bool GetReverseAtEnd(void) const; // [ property ]
  void SetReverseAtEnd(bool b);     // [ property ]

  float m_fAnimationSpeed = 1.0f; // [ property ]

protected:
  ezBitflags<ezTransformComponentFlags> m_Flags;
  ezTime m_AnimationTime;
};
