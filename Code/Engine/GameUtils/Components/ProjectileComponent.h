#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameUtils/Surfaces/SurfaceResource.h>

class EZ_GAMEUTILS_DLL ezProjectileComponentManager : public ezComponentManagerSimple<class ezProjectileComponent, true>
{
public:
  ezProjectileComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
};

/// \brief Defines what a projectile will do when it hits a surface
struct EZ_GAMEUTILS_DLL ezProjectileReaction
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Absorb,       ///< The projectile simply stops and is deleted
    Reflect,      ///< Bounces away along the reflected direction
    Attach,       ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough,  ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEUTILS_DLL, ezProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct EZ_GAMEUTILS_DLL ezProjectileSurfaceInteraction
{
  void SetSurface(const char* szSurface);
  const char* GetSurface() const;

  /// \brief The surface type (and derived ones) for which this interaction is used
  ezSurfaceResourceHandle m_hSurface;

  /// \brief How the projectile itself will react when hitting the surface type
  ezProjectileReaction::Enum m_Reaction;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEUTILS_DLL, ezProjectileSurfaceInteraction);

class EZ_GAMEUTILS_DLL ezProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProjectileComponent, ezComponent, ezProjectileComponentManager);

public:
  ezProjectileComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  float m_fMetersPerSecond;
  ezUInt8 m_uiCollisionLayer;
  ezHybridArray<ezProjectileSurfaceInteraction, 12> m_SurfaceInteractions;

private:

  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  ezInt32 FindSurfaceInteraction(const ezSurfaceResourceHandle& hSurface) const;

};



