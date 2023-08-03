#pragma once

#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>

struct ezMsgApplyRootMotion;

using ezJoltDefaultCharacterComponentManager = ezComponentManager<class ezJoltDefaultCharacterComponent, ezBlockStorageType::FreeList>;

class EZ_JOLTPLUGIN_DLL ezJoltDefaultCharacterComponent : public ezJoltCharacterControllerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltDefaultCharacterComponent, ezJoltCharacterControllerComponent, ezJoltDefaultCharacterComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltCharacterControllerComponent

public:
  ezJoltDefaultCharacterComponent();
  ~ezJoltDefaultCharacterComponent();

  enum class GroundState : ezUInt8
  {
    OnGround, ///< Character is touching the ground
    Sliding,  ///< Character is touching a steep surface and therefore slides downwards
    InAir,    ///< Character isn't touching any ground surface (may still touch a wall or ceiling)
  };

  ezAngle m_RotateSpeed = ezAngle::MakeFromDegree(90.0f); ///< [ property ] How many degrees per second the character turns
  float m_fShapeRadius = 0.25f;
  float m_fCylinderHeightCrouch = 0.9f;
  float m_fCylinderHeightStand = 1.7f;
  float m_fFootRadius = 0.15f;

  float m_fWalkSpeedCrouching = 0.5f;
  float m_fWalkSpeedStanding = 1.5f;
  float m_fWalkSpeedRunning = 3.5f;

  float m_fMaxStepUp = 0.25f;
  float m_fMaxStepDown = 0.25f;
  float m_fJumpImpulse = 5.0f;

  ezHashedString m_sWalkSurfaceInteraction;       ///< [ property ] The surface interaction to spawn regularly when walking
  ezSurfaceResourceHandle m_hFallbackWalkSurface; ///< [ property ] The surface type to use for interactions, when no other surface type is available
  float m_fWalkInteractionDistance = 1.0f;        ///< [ property ] How far the CC has to walk for spawning another surface interaction
  float m_fRunInteractionDistance = 3.0f;         ///< [ property ] How far the CC has to run for spawning another surface interaction

  void SetWalkSurfaceInteraction(const char* szSz) { m_sWalkSurfaceInteraction.Assign(szSz); }  // [ property ]
  const char* GetWalkSurfaceInteraction() const { return m_sWalkSurfaceInteraction.GetData(); } // [ property ]

  void SetFallbackWalkSurfaceFile(const char* szFile); // [ property ]
  const char* GetFallbackWalkSurfaceFile() const;      // [ property ]

  float m_fAirSpeed = 2.5f;    // [ property ]
  float m_fAirFriction = 0.5f; // [ property ]

  void SetHeadObjectReference(const char* szReference); // [ property ]

  void SetInputState(ezMsgMoveCharacterController& ref_msg);


  /// \brief Returns the current height of the entire capsule (crouching or standing).
  float GetCurrentCapsuleHeight() const;

  /// \brief Returns the current height of the cylindrical part of the capsule (crouching or standing).
  float GetCurrentCylinderHeight() const;

  /// \brief Returns the radius of the shape. This never changes at runtime.
  virtual float GetShapeRadius() const override;

  GroundState GetGroundState() const { return m_LastGroundState; }
  bool IsStandingOnGround() const { return m_LastGroundState == GroundState::OnGround; } // [ scriptable ]
  bool IsSlidingOnGround() const { return m_LastGroundState == GroundState::Sliding; }   // [ scriptable ]
  bool IsInAir() const { return m_LastGroundState == GroundState::InAir; }               // [ scriptable ]
  bool IsCrouching() const { return m_uiIsCrouchingBit; }                                // [ scriptable ]

  void TeleportCharacter(const ezVec3& vGlobalFootPosition);

  struct Config
  {
    bool m_bAllowJump = true;
    bool m_bAllowCrouch = true;
    bool m_bApplyGroundVelocity = true;
    ezVec3 m_vVelocity = ezVec3::MakeZero();
    float m_fPushDownForce = 0;
    ezHashedString m_sGroundInteraction;
    float m_fGroundInteractionDistanceThreshold = 1.0f;
    float m_fMaxStepUp = 0;
    float m_fMaxStepDown = 0;
  };

  virtual void DetermineConfig(Config& out_inputs);

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  virtual void OnApplyRootMotion(ezMsgApplyRootMotion& msg);

  virtual void UpdateCharacter() override;
  virtual void ApplyRotationZ();

  /// \brief Clears the input states to neutral values
  void ResetInputState();

  void ResetInternalState();

  /// \brief Creates a new shape with the given height (and fixed radius)
  virtual JPH::Ref<JPH::Shape> MakeNextCharacterShape() override;

  void ApplyCrouchState();
  void InteractWithSurfaces(const ContactPoint& contact, const Config& cfg);

  void StoreLateralVelocity();
  void ClampLateralVelocity();
  void MoveHeadObject();
  void DebugVisualizations();

  void CheckFeet();

  GroundState m_LastGroundState = GroundState::InAir;

  ezUInt8 m_uiInputJumpBit : 1;
  ezUInt8 m_uiInputCrouchBit : 1;
  ezUInt8 m_uiInputRunBit : 1;
  ezUInt8 m_uiIsCrouchingBit : 1;
  ezAngle m_InputRotateZ;
  ezVec2 m_vInputDirection = ezVec2::MakeZero();
  float m_fVelocityUp = 0.0f;
  float m_fNextCylinderHeight = 0;
  float m_fAccumulatedWalkDistance = 0.0f;
  ezVec2 m_vVelocityLateral = ezVec2::MakeZero();
  ezTransform m_PreviousTransform;
  bool m_bFeetOnSolidGround = false;

  float m_fCurrentCylinderHeight = 0;

  float m_fHeadHeightOffset = 0.0f;
  float m_fHeadTargetHeight = 0.0f;
  ezGameObjectHandle m_hHeadObject;

  ezVec3 m_vAbsoluteRootMotion = ezVec3::MakeZero();

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;

private:
  const char* DummyGetter() const { return nullptr; }
};
