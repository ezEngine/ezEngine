#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

struct ezMsgAnimationReachedEnd;

//////////////////////////////////////////////////////////////////////////

using ezFollowSplineComponentManager = ezComponentManagerSimple<class ezFollowSplineComponent, ezComponentUpdateType::WhenSimulating>;

struct EZ_GAMEENGINE_DLL ezFollowSplineMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    OnlyPosition,
    AlignUpZ,
    FullRotation,

    Default = OnlyPosition
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezFollowSplineMode)

/// \brief This component makes the ezGameObject, that it is attached to, move along a spline defined by an ezSplineComponent.
///
/// Build a spline using an ezSplineComponent and ezSplineNodeComponents.
/// Then attach an ezFollowSplineComponent to a free-standing ezGameObject and reference the object with the ezSplineComponent in it.
///
/// During simulation the ezFollowSplineComponent will now move and rotate its owner object such that it moves along the spline.
///
/// The start location of the 'hook' (the object with the ezFollowSplineComponent on it) may be anywhere. It will be teleported
/// onto the spline. For many objects this is not a problem, but physically simulated objects may be very sensitive about this.
///
/// One option is to align the 'hook' perfectly with the start location.
/// You can achieve this, using the "Keep Simulation Changes" feature of the editor (simulate with zero speed, press K, stop simulation).
/// Another option is to instead delay the spawning of the object below the hook, by using an ezSpawnComponent next to the ezFollowSplineComponent,
/// and thus have the payload spawn only after the hook has been placed properly.
class EZ_GAMEENGINE_DLL ezFollowSplineComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFollowSplineComponent, ezComponent, ezFollowSplineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezFollowSplineComponent

public:
  ezFollowSplineComponent();
  ~ezFollowSplineComponent();

  /// \brief Sets the reference to the game object on which an ezSplineComponent should be attached.
  void SetSplineObject(const char* szReference);      // [ property ]

  ezEnum<ezPropertyAnimMode> m_Mode;                  ///< [ property ] How the spline should be traversed.
  ezEnum<ezFollowSplineMode> m_FollowMode;            ///< [ property ] How the transform of the follower should be affected by the spline.
  float m_fSpeed = 1.0f;                              ///< [ property ] How fast to move along the spline.
  float m_fLookAhead = 0.0f;                          ///< [ property ] How far along the spline to 'look ahead' to smooth the rotation. A small distance means rotations are very abrupt.
  float m_fSmoothing = 0.5f;                          ///< [ property ] How much to combine the current position with the new position. 0 to 1. At zero, the position follows the spline perfectly, but therefore also has very abrupt changes. With a lot of smoothing, the spline becomes very sluggish.
  float m_fTiltAmount = 5.0f;                         ///< [ property ] How much to tilt when turning.
  ezAngle m_MaxTilt = ezAngle::MakeFromDegree(30.0f); ///< [ property ] The max tilt angle of the object.

  /// \brief Distance along the spline at which the ezFollowSplineComponent should start off.
  void SetStartDistance(float fDistance);                     // [ property ]
  float GetStartDistance() const { return m_fStartDistance; } // [ property ]

  /// \brief Sets the current distance along the spline at which the ezFollowSplineComponent should be.
  void SetCurrentDistance(float fDistance);                       // [ scriptable ]
  float GetCurrentDistance() const { return m_fCurrentDistance; } // [ scriptable ]

  /// \brief Whether the component should move along the spline 'forwards' or 'backwards'
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// \brief Toggles the direction that it travels along the spline.
  void ToggleDirection(); // [ scriptable ]

  /// \brief Whether the component currently moves 'forwards' along the spline.
  ///
  /// Note that if the 'speed' property is negative, moving 'forwards' along the spline still means that it effectively moves backwards.
  bool IsDirectionForwards() const { return m_bIsRunningForwards; } // [ scriptable ]

  /// \brief Whether the component currently moves along the spline, at all.
  bool IsRunning() const { return m_bIsRunning; } // [ property ]

  /// \brief Whether to move along the spline or not.
  void SetRunning(bool bRunning); // [ property ]

protected:
  void Update(bool bForce = false);

  ezEventMessageSender<ezMsgAnimationReachedEnd> m_ReachedEndEvent; // [ event ]
  ezGameObjectHandle m_hSplineObject;                               // [ property ]

  float m_fStartDistance = 0.0f;                                    // [ property ]

  float m_fCurrentDistance = 0.0f;
  bool m_bLastStateValid = false;
  bool m_bIsRunning = true;
  bool m_bIsRunningForwards = true;
  ezVec3 m_vLastPosition;
  ezVec3 m_vLastForwardDir;
  ezVec3 m_vLastUpDir;
  ezAngle m_LastTiltAngle;

  const char* DummyGetter() const { return nullptr; }
};
