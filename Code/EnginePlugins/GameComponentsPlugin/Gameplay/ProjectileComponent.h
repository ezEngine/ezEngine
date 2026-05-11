#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
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

/// \brief Defines how the projectile owner orientation is updated on reflection / bounce.
struct EZ_GAMECOMPONENTS_DLL ezProjectileBounceOrientation
{
  using StorageType = ezInt8;

  enum Enum : StorageType
  {
    Spinning = 0,
    Reflection = 1,

    Default = Reflection
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMECOMPONENTS_DLL, ezProjectileBounceOrientation);

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

  /// \brief How much the rotation (spinning) of the owner object is affected by reflections/bounces about the surface
  /// Positive values correspond to the rotation due to object "getting stuck in the surface"
  /// Negative values correspond to the rotation due to object "sliding over the surface"
  /// Larger by magnitude values correspond to smaller rotations.
  /// If zero, than the rotation of the owner is not changed during reflection/bounces
  /// Generally, it is an analogue of mass but for rotational movement
  float m_fInertiaRatio = 5.0f;
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

  /// If greater than zero, a sphere shape query is used for collision detection
  /// otherwise raycasting is used and projectile is treated like a "dot"
  float m_fRadius; // [ property ]

  /// Defines how reflections / bounces rotate the owner object.
  ezProjectileBounceOrientation::Enum m_BounceOrientation = ezProjectileBounceOrientation::Reflection; // [ property ]

  /// Velocity ratio below which a bounced projectile is considered static.
  float m_fStaticVelocityRatio = 0.05f; // [ property ]

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
  void ApplyReflectionRotation(const ezVec3& vCurDirection, const ezVec3& vSurfaceNormal);
  void ApplySpinningRotation(const ezProjectileSurfaceInteraction& interaction, const ezPhysicsCastResult& castResult, const ezVec3& vPositionOnReflection, const ezVec3& vCurDirection, const ezVec3& vNewVelocity);
  bool QueryCollision(const ezPhysicsWorldModuleInterface& physicsInterface, ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDirection, float fDistance, const ezPhysicsQueryParameters& queryParams) const;
  bool ShouldStopProjectile(const ezPhysicsWorldModuleInterface& physicsInterface, const ezPhysicsCastResult& castResult, const ezVec3& vVelocity);


  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  ezInt32 FindSurfaceInteraction(const ezSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const ezSurfaceResourceHandle& hSurface, ezGameObjectHandle hObject, const ezVec3& vPos, const ezVec3& vNormal, const ezVec3& vDirection, const char* szInteraction);

  ezVec3 m_vVelocity;
};
