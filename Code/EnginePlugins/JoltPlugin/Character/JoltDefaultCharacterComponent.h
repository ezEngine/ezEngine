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
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

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
    Jumping,  ///< Character is jumping up, not yet falling down
    Falling,  ///< Character isn't touching any ground surface (may still touch a wall or ceiling)
  };

  ezAngle m_RotateSpeed = ezAngle::Degree(90.0f); ///< [ property ] How many degrees per second the character turns
  float m_fShapeRadius = 0.25f;
  float m_fCylinderHeightCrouch = 0.9f;
  float m_fCylinderHeightStand = 1.7f;

  float m_fWalkSpeedCrouching = 0.5f;
  float m_fWalkSpeedStanding = 1.5f;
  float m_fWalkSpeedRunning = 3.5f;

  float m_fMaxStepHeight = 0.25f;
  float m_fJumpImpulse = 5.0f;

  ezHashedString m_sWalkSurfaceInteraction;       ///< [ property ] The surface interaction to spawn regularly when walking
  ezSurfaceResourceHandle m_hFallbackWalkSurface; ///< [ property ] The surface type to use for interactions, when no other surface type is available
  float m_fWalkInteractionDistance = 1.0f;        ///< [ property ] How far the CC has to walk for spawning another surface interaction
  float m_fRunInteractionDistance = 3.0f;         ///< [ property ] How far the CC has to run for spawning another surface interaction

  void SetWalkSurfaceInteraction(const char* sz) { m_sWalkSurfaceInteraction.Assign(sz); }      // [ property ]
  const char* GetWalkSurfaceInteraction() const { return m_sWalkSurfaceInteraction.GetData(); } // [ property ]

  void SetFallbackWalkSurfaceFile(const char* szFile); // [ property ]
  const char* GetFallbackWalkSurfaceFile() const;      // [ property ]

  float m_fAirSpeed = 2.5f;    // [ property ]
  float m_fAirFriction = 0.5f; // [ property ]

  void SetHeadObjectReference(const char* szReference); // [ property ]

  void SetInputState(ezMsgMoveCharacterController& msg);


  /// \brief Returns the current height of the entire capsule (crouching or standing).
  float GetCurrentCapsuleHeight() const;

  /// \brief Returns the current height of the cylindrical part of the capsule (crouching or standing).
  float GetCurrentCylinderHeight() const;

  /// \brief Returns the radius of the shape. This never changes at runtime.
  float GetShapeRadius() const;

  /// \brief Returns the current (crouching/standing) height of the capsule plus the step offset (the character floats above the ground)
  float GetCurrentTotalHeight() const;

  GroundState GetGroundState() const { return m_LastGroundState; }
  bool IsStandingOnGround() const { return m_LastGroundState == GroundState::OnGround; }                                  // [ scriptable ]
  bool IsSlidingOnGround() const { return m_LastGroundState == GroundState::Sliding; }                                    // [ scriptable ]
  bool IsInAir() const { return m_LastGroundState == GroundState::Jumping || m_LastGroundState == GroundState::Falling; } // [ scriptable ]
  bool IsCrouching() const { return m_IsCrouchingBit; }                                                                   // [ scriptable ]

  void TeleportCharacter(const ezVec3& vGlobalFootPosition);

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  virtual void OnApplyRootMotion(ezMsgApplyRootMotion& msg);

  virtual void UpdateCharacter() override;

  virtual void Update_OnGround();
  virtual void Update_InAir();
  virtual void Update_Sliding();
  virtual void ApplyRotationZ();

  /// \brief Clears the input states to neutral values
  void ResetInputState();

  void ResetInternalState();

  /// \brief Creates a new shape with the given height (and fixed radius)
  virtual JPH::Ref<JPH::Shape> MakeNextCharacterShape() override;

  /// \brief Checks what the ground state would be at the given position. If there is any contact, returns the most interesting contact point.
  GroundState DetermineGroundState(ContactPoint& out_Contact, const ezVec3& vFootPosition, float fAllowedStepUp, float fAllowedStepDown, float fCylinderRadius) const;

  GroundState DetermineCurrentState(bool bAllowStepDown);

  ezVec3 ComputeInputVelocity_OnGround() const;
  ezVec3 ComputeInputVelocity_InAir() const;

  /// \brief Modifies the up/down velocity such that the character sticks to the ground when walking down stairs, and steps up when encountering small obstacles.
  void AdjustVelocityToStepUpOrDown(float& fVelocityUp, const ContactPoint& groundContact) const;

  void ApplyCrouchState();
  void InteractWithSurfaces(const ezVec2& vWalkAmount, const ezVec3& vStartPos, GroundState groundState, const ContactPoint& contact);

  void ClampUpVelocity();

  void VisualizeShape();

  void StoreLateralVelocity();
  void ClampLateralVelocity();

  ezHybridArray<ContactPoint, 32> m_CurrentContacts;
  GroundState m_LastGroundState = GroundState::Falling;

  ezUInt8 m_InputJumpBit : 1;
  ezUInt8 m_InputCrouchBit : 1;
  ezUInt8 m_InputRunBit : 1;
  ezUInt8 m_IsCrouchingBit : 1;
  ezAngle m_InputRotateZ;
  ezVec2 m_InputDirection = ezVec2::ZeroVector();
  float m_fVelocityUp = 0.0f;
  float m_fNextCylinderHeight = 0;
  float m_fAccumulatedWalkDistance = 0.0f;
  ezVec2 m_vVelocityLateral = ezVec2::ZeroVector();
  ezTransform m_PreviousTransform;

  float m_fCurrentCylinderHeight = 0;

  float m_fHeadHeightOffset = 0.0f;
  float m_fHeadTargetHeight = 0.0f;
  ezGameObjectHandle m_hHeadObject;

  ezVec3 m_vAbsoluteRootMotion = ezVec3::ZeroVector();

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;

private:
  const char* DummyGetter() const { return nullptr; }
};
