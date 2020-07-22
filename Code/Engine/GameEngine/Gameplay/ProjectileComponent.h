#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Physics/SurfaceResource.h>

struct ezMsgComponentInternalTrigger;

typedef ezComponentManagerSimple<class ezProjectileComponent, ezComponentUpdateType::WhenSimulating> ezProjectileComponentManager;

/// \brief Defines what a projectile will do when it hits a surface
struct EZ_GAMEENGINE_DLL ezProjectileReaction
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct EZ_GAMEENGINE_DLL ezProjectileSurfaceInteraction
{
  void SetSurface(const char* szSurface);
  const char* GetSurface() const;

  /// \brief The surface type (and derived ones) for which this interaction is used
  ezSurfaceResourceHandle m_hSurface;

  /// \brief How the projectile itself will react when hitting the surface type
  ezProjectileReaction::Enum m_Reaction;

  /// \brief Which interaction should be triggered. See ezSurfaceResource.
  ezString m_sInteraction;

  /// \brief The force (or rather impulse) that is applied on the object
  float m_fImpulse = 0.0f;

  /// \brief How much damage to do on this type of surface. Send via ezMsgDamage
  float m_fDamage = 0.0f;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezProjectileSurfaceInteraction);

class EZ_GAMEENGINE_DLL ezProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProjectileComponent, ezComponent, ezProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezProjectileComponent

public:
  ezProjectileComponent();
  ~ezProjectileComponent();

  float m_fMetersPerSecond;                   ///< [ property ] The speed at which the projectile flies
  float m_fGravityMultiplier;                 ///< [ property ] If 0, the projectile is not affected by gravity.
  ezUInt8 m_uiCollisionLayer;                 ///< [ property ]
  ezTime m_MaxLifetime;                       ///< [ property ] After this time the projectile is killed, if it didn't die already
  ezSurfaceResourceHandle m_hFallbackSurface; ///< [ property ]
  ezHybridArray<ezProjectileSurfaceInteraction, 12> m_SurfaceInteractions; ///< [ property ]

  void SetTimeoutPrefab(const char* szPrefab); // [ property ]
  const char* GetTimeoutPrefab() const;        // [ property ]

  void SetFallbackSurfaceFile(const char* szFile); // [ property ]
  const char* GetFallbackSurfaceFile() const;      // [ property ]

private:
  void Update();
  void OnTriggered(ezMsgComponentInternalTrigger& msg); // [ msg handler ]

  ezPrefabResourceHandle m_hTimeoutPrefab; ///< Spawned when the projectile is killed due to m_MaxLifetime coming to an end

  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  ezInt32 FindSurfaceInteraction(const ezSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const ezSurfaceResourceHandle& hSurface, ezGameObjectHandle hObject, const ezVec3& vPos, const ezVec3& vNormal,
    const ezVec3& vDirection, const char* szInteraction);

  ezVec3 m_vVelocity;
};
