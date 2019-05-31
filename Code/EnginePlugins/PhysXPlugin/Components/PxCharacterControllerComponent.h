#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgCollision;
typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

class ezPxCharacterControllerComponentManager
  : public ezComponentManager<class ezPxCharacterControllerComponent, ezBlockStorageType::Compact>
{
public:
  ezPxCharacterControllerComponentManager(ezWorld* pWorld);
  ~ezPxCharacterControllerComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
};

class EZ_PHYSXPLUGIN_DLL ezPxCharacterControllerComponent : public ezCharacterControllerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterControllerComponent, ezCharacterControllerComponent, ezPxCharacterControllerComponentManager);

public:
  ezPxCharacterControllerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void RawMove(const ezVec3& vMove) override;

  void Update();

  virtual void OnSimulationStarted() override;

  // ************************************* PROPERTIES ***********************************
public:
  float m_fWalkSpeed;    ///< How many meters the character walks per second
  float m_fRunSpeed;     ///< How many meters the character runs per second
  float m_fCrouchSpeed;  ///< How many meters the character walks per second while crouching
  float m_fAirSpeed;     ///< How fast the character can change direction while not standing on solid round
  float m_fAirFriction;  ///< How much damping is applied to the velocity when the character is jumping
  ezAngle m_RotateSpeed; ///< How many degrees per second the character turns
  float m_fJumpImpulse;

  float m_fPushingForce; ///< force to push other rigid bodies
  ezHashedString m_sWalkSurfaceInteraction;
  ezSurfaceResourceHandle m_hFallbackWalkSurface;
  float m_fWalkInteractionDistance;
  float m_fRunInteractionDistance;

  void SetWalkSurfaceInteraction(const char* sz) { m_sWalkSurfaceInteraction.Assign(sz); }
  const char* GetWalkSurfaceInteraction() const { return m_sWalkSurfaceInteraction.GetData(); }

  void SetFallbackWalkSurfaceFile(const char* szFile);
  const char* GetFallbackWalkSurfaceFile() const;

protected:
  // ************************************* FUNCTIONS *****************************

public:
  virtual void MoveCharacter(ezMsgMoveCharacterController& msg) override;
  void OnCollision(ezMsgCollision& msg);

protected:
  ezComponentHandle m_hProxy;

  enum InputStateBits : ezUInt8
  {
    Jump = EZ_BIT(0),
    Crouch = EZ_BIT(1),
    Run = EZ_BIT(2),
  };

  ezUInt8 m_InputStateBits;
  ezVec3 m_vRelativeMoveDirection;
  ezAngle m_RotateZ;
  bool m_bWantsCrouch = false;

  float m_fVelocityUp = 0.0f;
  ezVec3 m_vVelocityLateral = ezVec3(0.0f);

  float m_fAccumulatedWalkDistance = 0.0f;
};
