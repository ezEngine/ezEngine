#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgCollision;
struct ezMsgApplyRootMotion;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

class ezPxCharacterControllerComponentManager : public ezComponentManager<class ezPxCharacterControllerComponent, ezBlockStorageType::Compact>
{
public:
  ezPxCharacterControllerComponentManager(ezWorld* pWorld);
  ~ezPxCharacterControllerComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
};

class EZ_PHYSXPLUGIN_DLL ezPxCharacterControllerComponent : public ezCharacterControllerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterControllerComponent, ezCharacterControllerComponent, ezPxCharacterControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCharacterControllerComponent

public:
  virtual void RawMove(const ezVec3& vMoveDeltaGlobal) override;
  virtual void MoveCharacter(ezMsgMoveCharacterController& ref_msg) override;
  virtual void TeleportCharacter(const ezVec3& vGlobalFootPos) override;
  virtual bool IsDestinationUnobstructed(const ezVec3& vGlobalFootPos, float fCharacterHeight) override;
  virtual bool IsTouchingGround() override;
  virtual bool IsCrouching() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxCharacterControllerComponent
  virtual void OnApplyRootMotion(ezMsgApplyRootMotion& ref_msg);

public:
  ezPxCharacterControllerComponent();

  void OnCollision(ezMsgCollision& ref_msg);                                                    // [ msg handler ]

  float m_fWalkSpeed = 5.0f;                                                                    ///< [ property ] How many meters the character walks per second
  float m_fRunSpeed = 15.0f;                                                                    ///< [ property ] How many meters the character runs per second
  float m_fCrouchHeight = 0.2f;                                                                 ///< [ property ] How many meters the character walks per second while crouching
  float m_fCrouchSpeed = 2.0f;                                                                  ///< [ property ] How many meters the character walks per second while crouching
  float m_fAirSpeed = 2.5f;                                                                     ///< [ property ] How fast the character can change direction while not standing on solid round
  float m_fAirFriction = 0.5f;                                                                  ///< [ property ] How much damping is applied to the velocity when the character is jumping
  ezAngle m_RotateSpeed = ezAngle::MakeFromDegree(90.0f);                                       ///< [ property ] How many degrees per second the character turns
  float m_fJumpImpulse = 6.0f;                                                                  ///< [ property ] How high the character will be able to jump
  float m_fPushingForce = 500.0f;                                                               ///< [ property ] Force to push other rigid bodies
  ezHashedString m_sWalkSurfaceInteraction;                                                     ///< [ property ] The surface interaction to spawn regularly when walking
  ezSurfaceResourceHandle m_hFallbackWalkSurface;                                               ///< [ property ] The surface type to use for interactions, when no other surface type is available
  float m_fWalkInteractionDistance = 1.0f;                                                      ///< [ property ] How far the CC has to walk for spawning another surface interaction
  float m_fRunInteractionDistance = 3.0f;                                                       ///< [ property ] How far the CC has to run for spawning another surface interaction

  void SetWalkSurfaceInteraction(const char* szSz) { m_sWalkSurfaceInteraction.Assign(szSz); }  // [ property ]
  const char* GetWalkSurfaceInteraction() const { return m_sWalkSurfaceInteraction.GetData(); } // [ property ]

  void SetFallbackWalkSurfaceFile(const char* szFile);                                          // [ property ]
  const char* GetFallbackWalkSurfaceFile() const;                                               // [ property ]

  void SetHeadObjectReference(const char* szReference);                                         // [ property ]

protected:
  void Update();

  enum InputStateBits : ezUInt8
  {
    Jump = EZ_BIT(0),
    Crouch = EZ_BIT(1),
    Run = EZ_BIT(2),
  };

  ezUInt8 m_uiInputStateBits = 0;
  ezComponentHandle m_hCharacterShape;
  ezVec3 m_vRelativeMoveDirection = ezVec3::MakeZero();
  ezVec3 m_vAbsoluteRootMotion = ezVec3::MakeZero();
  ezAngle m_RotateZ;
  bool m_bIsTouchingGround = true;
  bool m_bWantsCrouch = false;
  bool m_bIsCrouching = false;
  bool m_bWantsTeleport = false;
  float m_fStandingHeight = 1.0f;
  float m_fVelocityUp = 0.0f;
  ezVec3 m_vVelocityLateral = ezVec3::MakeZero();
  float m_fAccumulatedWalkDistance = 0.0f;
  ezVec3 m_vTeleportTo;

  float m_fHeadHeightOffset = 0.0f;
  float m_fHeadTargetHeight = 0.0f;
  ezGameObjectHandle m_hHeadObject;

private:
  const char* DummyGetter() const { return nullptr; }
};
