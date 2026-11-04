#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>

class ShipComponent;
using ShipComponentManager = ezComponentManagerSimple<ShipComponent, ezComponentUpdateType::WhenSimulating>;

/// The player's ship: reads the input actions, flies with inertia, fires projectiles and manages its power-ups.
///
/// The ship's prefab can have child objects with these names:
///   'Visual': gets banked when turning and blinks while invulnerable.
///   'Thrust': receives ezMsgSetPlaying while the engine fires. If it has a sound component, its volume is faded along.
///   'Shield': only active while the ship has a shield, so it should be inactive in the prefab.
///
/// Whether the ship got hit is decided by AsteroidsGameComponent, which then calls Hit().
class ShipComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ShipComponent, ezComponent, ShipComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ShipComponent

public:
  ShipComponent();
  ~ShipComponent();

  void Update();

  /// True while the ship can't be damaged, for example right after spawning.
  bool IsInvulnerable() const { return m_InvulnerableTime.IsPositive(); }

  /// Makes the ship immune to hits for the given duration.
  void SetInvulnerable(ezTime duration) { m_InvulnerableTime = duration; }

  /// A shield absorbs the hit and plays the shield sound. Returns true if the ship did not survive.
  bool Hit();

  void GiveShield();
  void GiveTripleShot();

  const ezVec3& GetVelocity() const { return m_vVelocity; }

  ezPrefabResourceHandle m_hProjectilePrefab; // [ property ]
  ezPrefabResourceHandle m_hExplosionPrefab;  // [ property ]
  ezString m_sFireSound;                      // [ property ]
  ezString m_sShieldHitSound;                 // [ property ]

  float m_fRadius = 1.5f;                                    // [ property ] the collision radius
  float m_fThrust = 45.0f;                                   // [ property ] acceleration in units per second squared
  float m_fMaxSpeed = 38.0f;                                 // [ property ]
  float m_fDrag = 0.45f;                                     // [ property ] fraction of the speed that is lost per second
  ezAngle m_TurnSpeed = ezAngle::MakeFromDegree(250.0f);     // [ property ] rotation per second
  ezTime m_FireInterval = ezTime::MakeFromSeconds(0.16);     // [ property ] delay between shots while the fire button is held
  ezTime m_TripleShotDuration = ezTime::MakeFromSeconds(10); // [ property ]

private:
  void Fly(float fTimeDiff, float fTurn, float fThrust);
  void UpdateVisuals(float fTimeDiff, float fTurn);
  void UpdateThrustEffect(float fTimeDiff, bool bThrusting);
  void FireProjectiles();
  void SpawnProjectile(ezAngle yaw);
  void PlaySound(const ezString& sSound, float fPitch = 1.0f);

  ezVec3 m_vVelocity = ezVec3::MakeZero();
  ezTime m_FireCooldown;
  ezTime m_InvulnerableTime;
  ezTime m_TripleShotTime;
  float m_fBank = 0.0f;
  float m_fThrustVolume = 0.0f;
  bool m_bThrusting = false;
  bool m_bHasShield = false;

  ezGameObjectHandle m_hVisual;
  ezGameObjectHandle m_hThrust;
  ezGameObjectHandle m_hShield;
  ezQuat m_qVisualBaseRotation = ezQuat::MakeIdentity();
};
