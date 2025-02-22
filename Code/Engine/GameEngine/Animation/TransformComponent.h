#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief Internal flags for the current state of a transform component
struct ezTransformComponentFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    None = 0,
    Running = EZ_BIT(0),           ///< Start state for the CurrentlyRunning flag
    AutoReturnStart = EZ_BIT(1),   ///< When reaching the start point, the transform should automatically turn around
    AutoReturnEnd = EZ_BIT(2),     ///< When reaching the end point, the transform should automatically turn around
    CurrentlyRunning = EZ_BIT(3),  ///< The component is currently modifying the transform
    AnimationReversed = EZ_BIT(5), ///< The animation playback is currently in reverse
    Default = Running | AutoReturnStart | AutoReturnEnd
  };

  struct Bits
  {
    StorageType Running : 1;
    StorageType AutoReturnStart : 1;
    StorageType AutoReturnEnd : 1;
    StorageType CurrentlyRunning : 1;
    StorageType Unused2 : 1;
    StorageType AnimationReversed : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezTransformComponentFlags);

/// \brief Base class for some components that modify an object's transform.
class EZ_GAMEENGINE_DLL ezTransformComponent : public ezComponent
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransformComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezTransformComponent

public:
  ezTransformComponent();
  ~ezTransformComponent();

  /// \brief Sets the animation to be played forwards or backwards.
  ///
  /// \note Does not start the animation, if it is currently not running.
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// \brief Toggles the directon of the animation.
  ///
  /// \note Does not start the animation, if it is currently not running.
  void ToggleDirection(); // [ scriptable ]

  /// \brief Returns whether the animation is currently being played forwards or backwards.
  bool IsDirectionForwards() const; // [ scriptable ]

  /// \brief Returns whether the animation is currently being played back or paused.
  bool IsRunning(void) const; // [ property ]

  /// \brief Starts or stops animation playback.
  void SetRunning(bool bRunning); // [ property ]

  /// \brief Returns whether the animation would turn around automatically when reaching the start point.
  bool GetReverseAtStart(void) const; // [ property ]
  void SetReverseAtStart(bool b);     // [ property ]

  /// \brief Returns whether the animation would turn around automatically when reaching the end point.
  bool GetReverseAtEnd(void) const; // [ property ]
  void SetReverseAtEnd(bool b);     // [ property ]

  /// \brief The speed at which the animation should be played back.
  float m_fAnimationSpeed = 1.0f; // [ property ]

protected:
  ezBitflags<ezTransformComponentFlags> m_Flags;
  ezTime m_AnimationTime;
};
