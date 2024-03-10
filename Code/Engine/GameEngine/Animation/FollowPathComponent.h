#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PathComponent.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

struct ezMsgAnimationReachedEnd;

//////////////////////////////////////////////////////////////////////////

using ezFollowPathComponentManager = ezComponentManagerSimple<class ezFollowPathComponent, ezComponentUpdateType::WhenSimulating>;

struct EZ_GAMEENGINE_DLL ezFollowPathMode
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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezFollowPathMode)

/// \brief This component makes the ezGameObject, that it is attached to, move along a path defined by an ezPathComponent.
///
/// Build a path using an ezPathComponent and ezPathNodeComponents.
/// Then attach an ezFollowPathComponent to a free-standing ezGameObject and reference the object with the ezPathComponent in it.
///
/// During simulation the ezFollowPathComponent will now move and rotate its owner object such that it moves along the path.
///
/// The start location of the 'hook' (the object with the ezFollowPathComponent on it) may be anywhere. It will be teleported
/// onto the path. For many objects this is not a problem, but physically simulated objects may be very sensitive about this.
///
/// One option is to align the 'hook' perfectly with the start location.
/// You can achieve this, using the "Keep Simulation Changes" feature of the editor (simulate with zero speed, press K, stop simulation).
/// Another option is to instead delay the spawning of the object below the hook, by using an ezSpawnComponent next to the ezFollowPathComponent,
/// and thus have the payload spawn only after the hook has been placed properly.
class EZ_GAMEENGINE_DLL ezFollowPathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFollowPathComponent, ezComponent, ezFollowPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezFollowPathComponent

public:
  ezFollowPathComponent();
  ~ezFollowPathComponent();

  /// \brief Sets the reference to the game object on which an ezPathComponent should be attached.
  void SetPathObject(const char* szReference);        // [ property ]

  ezEnum<ezPropertyAnimMode> m_Mode;                  ///< [ property ] How the path should be traversed.
  ezEnum<ezFollowPathMode> m_FollowMode;              ///< [ property ] How the transform of the follower should be affected by the path.
  float m_fSpeed = 1.0f;                              ///< [ property ] How fast to move along the path.
  float m_fLookAhead = 1.0f;                          ///< [ property ] How far along the path to 'look ahead' to smooth the rotation. A small distance means rotations are very abrupt.
  float m_fSmoothing = 0.5f;                          ///< [ property ] How much to combine the current position with the new position. 0 to 1. At zero, the position follows the path perfectly, but therefore also has very abrupt changes. With a lot of smoothing, the path becomes very sluggish.
  float m_fTiltAmount = 5.0f;                         ///< [ property ] How much to tilt when turning.
  ezAngle m_MaxTilt = ezAngle::MakeFromDegree(30.0f); ///< [ property ] The max tilt angle of the object.

  /// \brief Distance along the path at which the ezFollowPathComponent should start off.
  void SetDistanceAlongPath(float fDistance); // [ property ]
  float GetDistanceAlongPath() const;         // [ property ]

  /// \brief Whether the component should move along the path 'forwards' or 'backwards'
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// \brief Toggles the direction that it travels along the path.
  void ToggleDirection(); // [ scriptable ]

  /// \brief Whether the component currently moves 'forwards' along the path.
  ///
  /// Note that if the 'speed' property is negative, moving 'forwards' along the path still means that it effectively moves backwards.
  bool IsDirectionForwards() const; // [ scriptable ]

  /// \brief Whether the component currently moves along the path, at all.
  bool IsRunning() const; // [ property ]

  /// \brief Whether to move along the path or not.
  void SetRunning(bool bRunning); // [ property ]

protected:
  void Update(bool bForce = false);

  ezEventMessageSender<ezMsgAnimationReachedEnd> m_ReachedEndEvent; // [ event ]
  ezGameObjectHandle m_hPathObject;                                 // [ property ]
  ezPathComponent::LinearSampler m_PathSampler;

  float m_fStartDistance = 0.0f;                                    // [ property ]
  bool m_bLastStateValid = false;
  bool m_bIsRunning = true;
  bool m_bIsRunningForwards = true;
  ezVec3 m_vLastPosition;
  ezVec3 m_vLastTargetPosition;
  ezVec3 m_vLastUpDir;
  ezAngle m_LastTiltAngle;

  const char* DummyGetter() const { return nullptr; }
};
