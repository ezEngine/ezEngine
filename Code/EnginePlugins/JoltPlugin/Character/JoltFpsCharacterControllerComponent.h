#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgApplyRootMotion;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

struct ezCCContactPoint
{
  float m_fCastFraction;
  ezVec3 m_vPosition;
  ezVec3 m_vSurfaceNormal;
  ezVec3 m_vContactNormal;
  JPH::BodyID m_BodyID;
};

class ezJoltFpsCharacterControllerComponentManager : public ezComponentManager<class ezJoltFpsCharacterControllerComponent, ezBlockStorageType::Compact>
{
public:
  ezJoltFpsCharacterControllerComponentManager(ezWorld* pWorld);
  ~ezJoltFpsCharacterControllerComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
};

EZ_DECLARE_FLAGS(ezUInt32, ezJoltCCDebugFlags, VisQueryShape, VisContactPoints, VisGroundContact, VisVelocity);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltCCDebugFlags);

class EZ_JOLTPLUGIN_DLL ezJoltFpsCharacterControllerComponent : public ezCharacterControllerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltFpsCharacterControllerComponent, ezCharacterControllerComponent, ezJoltFpsCharacterControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////

public:
  float m_fMass = 80.0f;          ///< [ property ] mass is used to calculate pushing force from other rigid bodies
  ezUInt8 m_uiCollisionLayer = 0; ///< [ property ] What other geometry the CC will collide with
  float m_fCapsuleHeight = 1.0f;  ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleRadius = 0.25f; ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius

  ezBitflags<ezJoltCCDebugFlags> m_DebugFlags;

  float GetCurrentHeightValue() const { return m_fCurrentHeightValue; } // [ scriptable ]
  float GetCurrentTotalHeight() const;

  bool TestShapeSweep(ezPhysicsCastResult& out_sweepResult, const ezVec3& vDirGlobal, float fDistance) const;
  bool TestShapeOverlap(const ezVec3& vGlobalFootPos, float fNewHeightValue) const;
  bool CanResize(float fNewHeightValue) const;
  bool TryResize(float fNewHeightValue);

  JPH::Ref<JPH::Shape> CreateCharacterShape(float fCapsuleHeight) const;

  const ezJoltUserData* GetUserData() const;

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]

  float m_fCurrentHeightValue = 0.0f;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;

  //////////////////////////////////////////////////////////////////////////
  // ezCharacterControllerComponent

public:
  virtual void RawMove(const ezVec3& vMoveDeltaGlobal) override;
  virtual void MoveCharacter(ezMsgMoveCharacterController& msg) override;
  virtual void TeleportCharacter(const ezVec3& vGlobalFootPos) override;
  virtual bool IsDestinationUnobstructed(const ezVec3& vGlobalFootPos, float fCharacterHeight) override;
  virtual bool IsTouchingGround() override;
  virtual bool IsCrouching() override;

  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; }

  //////////////////////////////////////////////////////////////////////////
  // ezJoltFpsCharacterControllerComponent
  virtual void OnApplyRootMotion(ezMsgApplyRootMotion& msg);

public:
  ezJoltFpsCharacterControllerComponent();

  float m_fMaxStepHeight = 0.3f;

  ezAngle m_MaxClimbingSlope = ezAngle::Degree(40);
  bool m_bForceSlopeSliding = true;
  float m_fWalkSpeed = 5.0f;    ///< [ property ] How many meters the character walks per second
  float m_fRunSpeed = 15.0f;    ///< [ property ] How many meters the character runs per second
  float m_fCrouchHeight = 0.2f; ///< [ property ] How many meters the character walks per second while crouching
  float m_fCrouchSpeed = 2.0f;  ///< [ property ] How many meters the character walks per second while crouching
  // float m_fAirSpeed = 2.5f;                       ///< [ property ] How fast the character can change direction while not standing on solid round
  // float m_fAirFriction = 0.5f;                    ///< [ property ] How much damping is applied to the velocity when the character is jumping
  ezAngle m_RotateSpeed = ezAngle::Degree(90.0f); ///< [ property ] How many degrees per second the character turns
  float m_fJumpImpulse = 6.0f;                    ///< [ property ] How high the character will be able to jump
  // float m_fPushingForce = 500.0f;                 ///< [ property ] Force to push other rigid bodies
  // ezHashedString m_sWalkSurfaceInteraction;       ///< [ property ] The surface interaction to spawn regularly when walking
  // ezSurfaceResourceHandle m_hFallbackWalkSurface; ///< [ property ] The surface type to use for interactions, when no other surface type is available
  // float m_fWalkInteractionDistance = 1.0f;        ///< [ property ] How far the CC has to walk for spawning another surface interaction
  // float m_fRunInteractionDistance = 3.0f;         ///< [ property ] How far the CC has to run for spawning another surface interaction

  // void SetWalkSurfaceInteraction(const char* sz) { m_sWalkSurfaceInteraction.Assign(sz); }      // [ property ]
  // const char* GetWalkSurfaceInteraction() const { return m_sWalkSurfaceInteraction.GetData(); } // [ property ]

  // void SetFallbackWalkSurfaceFile(const char* szFile); // [ property ]
  // const char* GetFallbackWalkSurfaceFile() const;      // [ property ]

  // void SetHeadObjectReference(const char* szReference); // [ property ]

protected:
  void Update();
  void ResetInputState();
  void ApplyPhysicsTransform();
  void ApplyCrouchState();
  ezVec3 ComputeInputVelocity() const;

  enum class GroundState : ezUInt8
  {
    OnGround,
    Sliding,
    InAir
  };

  ezUInt8 m_InputJumpBit : 1;
  ezUInt8 m_InputCrouchBit : 1;
  ezUInt8 m_InputRunBit : 1;
  ezUInt8 m_IsCrouchingBit : 1;

  ezAngle m_RotateZ;
  ezVec3 m_vInputDirection;
  GroundState m_GroundState = GroundState::InAir;
  ezCCContactPoint m_GroundContact;
  float m_fVelocityUp = 0.0f;

  // float m_fHeadHeightOffset = 0.0f;
  // float m_fHeadTargetHeight = 0.0f;
  // ezGameObjectHandle m_hHeadObject;

  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
  JPH::Ref<JPH::Character> m_pController;

  ezVec3 ClampVelocityByContactPoints(ezVec3 vIntendedVelocity, const ezDynamicArray<ezCCContactPoint>& contacts) const;

  ezVec3 RestrictMovementByGeometry(const ezVec3& vIntendedMovement, float fCylinderHeight, float fCylinderRadius) const;

  static ezVec3 GetGroundVelocityAndPushDown(const ezCCContactPoint& contact, JPH::PhysicsSystem* pJoltSystem, float fPushGroundForce);

  GroundState DetermineGroundState(ezCCContactPoint& out_Contact, const ezVec3& vFootPosition, float fAllowedStepUp, float fAllowedStepDown, float fCylinderRadius) const;

  void CollectContacts(ezDynamicArray<ezCCContactPoint>* out_pContacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezVec3& vMoveDir) const;
  void CollectCastContacts(ezDynamicArray<ezCCContactPoint>* out_pContacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezVec3& vSweepDir) const;

  void DebugDraw_SideContactCylinder(const ezVec3& vCylinderBottom, const ezVec3& vCylinderTop, float fCylinderRadius) const;

  void AdjustVelocityToStepUpOrDown(float& fVelocityUp, const ezCCContactPoint& groundContact, float fTimeDiff) const;

private:
  const char* DummyGetter() const { return nullptr; }
};
