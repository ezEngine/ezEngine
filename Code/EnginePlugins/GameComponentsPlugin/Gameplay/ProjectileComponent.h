#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct ezMsgComponentInternalTrigger;

using ezProjectileComponentManager = ezComponentManagerSimple<class ezProjectileComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Defines what a projectile will do when it hits a surface
struct EZ_GAMECOMPONENTS_DLL ezProjectileReaction
{
  using StorageType = ezInt8;

  enum Enum : StorageType
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction. Maintains momentum.
    Bounce,      ///< Bounces away along the reflected direction. Loses momentum.
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMECOMPONENTS_DLL, ezProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct EZ_GAMECOMPONENTS_DLL ezProjectileSurfaceInteraction
{
  /// \brief The surface type (and derived ones) for which this interaction is used
  ezSurfaceResourceHandle m_hSurface;

  /// \brief How the projectile itself will react when hitting the surface type
  ezProjectileReaction::Enum m_Reaction;

  /// \brief Which interaction should be triggered. See ezSurfaceResource.
  ezString m_sInteraction;

  /// \brief Which impulse type to use.
  ezUInt8 m_uiImpulseType = 0;

  /// \brief The force (or rather impulse) that is applied on the object
  float m_fImpulse = 0.0f;

  /// \brief How much damage to do on this type of surface. Send via ezMsgDamage
  float m_fDamage = 0.0f;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMECOMPONENTS_DLL, ezProjectileSurfaceInteraction);

/// \brief Shoots a game object in a straight line and uses physics raycasts to detect hits.
///
/// When a raycast detects a hit, the surface information is used to determine how the projectile should proceed
/// and which prefab it should spawn as an effect.
class EZ_GAMECOMPONENTS_DLL ezProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProjectileComponent, ezComponent, ezProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezProjectileComponent

public:
  ezProjectileComponent();
  ~ezProjectileComponent();

  /// The speed at which the projectile flies.
  float m_fMetersPerSecond; // [ property ]

  /// If 0, the projectile is not affected by gravity.
  float m_fGravityMultiplier; // [ property ]

  // If true the death prefab will be spawned when the velocity goes under the threshold to be considered static
  bool m_bSpawnPrefabOnStatic; // [ property ]

  /// Defines which other physics objects the projectile will collide with.
  ezUInt8 m_uiCollisionLayer; // [ property ]

  /// A broad filter to ignore certain types of colliders.
  ezBitflags<ezPhysicsShapeType> m_ShapeTypesToHit; // [ property ]

  /// After this time the projectile is removed, if it didn't hit anything yet.
  ezTime m_MaxLifetime; // [ property ]

  /// If the projectile hits something that has no valid surface, this surface is used instead.
  ezSurfaceResourceHandle m_hFallbackSurface; // [ property ]

  /// Specifies how the projectile interacts with different surface types.
  ezHybridArray<ezProjectileSurfaceInteraction, 12> m_SurfaceInteractions; // [ property ]

  /// \brief If the projectile reaches its maximum lifetime it can spawn this prefab.
  ezPrefabResourceHandle m_hDeathPrefab;           // [ property ]

  void SetFallbackSurfaceFile(ezStringView sFile); // [ property ]
  ezStringView GetFallbackSurfaceFile() const;     // [ property ]

private:
  void Update();
  void OnTriggered(ezMsgComponentInternalTrigger& msg); // [ msg handler ]

  void SpawnDeathPrefab();


  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  ezInt32 FindSurfaceInteraction(const ezSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const ezSurfaceResourceHandle& hSurface, ezGameObjectHandle hObject, const ezVec3& vPos, const ezVec3& vNormal, const ezVec3& vDirection, const char* szInteraction);

  ezVec3 m_vVelocity;
};
