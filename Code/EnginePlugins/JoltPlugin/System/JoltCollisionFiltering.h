#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Basics.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <Physics/Body/BodyFilter.h>

class ezCollisionFilterConfig;

namespace JPH
{
  using ObjectLayer = ezUInt16;
} // namespace JPH

enum class ezJoltBroadphaseLayer : ezUInt8
{
  Static,
  Dynamic,
  Query,
  Trigger,
  Character,
  Ragdoll,
  Rope,
  Cloth,
  Debris,

  ENUM_COUNT
};

namespace ezJoltCollisionFiltering
{
  /// \brief Constructs the JPH::ObjectLayer value from the desired collision group index and the broadphase into which the object shall be sorted
  EZ_JOLTPLUGIN_DLL JPH::ObjectLayer ConstructObjectLayer(ezUInt8 uiCollisionGroup, ezJoltBroadphaseLayer broadphase);

  /// \brief Returns the (hard-coded) collision mask that determines which other broad-phases to collide with.
  EZ_JOLTPLUGIN_DLL ezUInt32 GetBroadphaseCollisionMask(ezJoltBroadphaseLayer broadphase);

}; // namespace ezJoltCollisionFiltering


class EZ_JOLTPLUGIN_DLL ezJoltObjectToBroadphaseLayer final : public JPH::BroadPhaseLayerInterface
{
public:
  virtual ezUInt32 GetNumBroadPhaseLayers() const override;

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
};

class EZ_JOLTPLUGIN_DLL ezJoltBroadPhaseLayerFilter final : public JPH::BroadPhaseLayerFilter
{
public:
  ezJoltBroadPhaseLayerFilter(ezBitflags<ezPhysicsShapeType> shapeTypes)
  {
    m_uiCollisionMask = shapeTypes.GetValue();
  }

  ezUInt32 m_uiCollisionMask = 0;

  virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
  {
    return (EZ_BIT(static_cast<ezUInt8>(inLayer)) & m_uiCollisionMask) != 0;
  }
};

class EZ_JOLTPLUGIN_DLL ezJoltObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
  ezUInt32 m_uiCollisionLayer = 0;

  ezJoltObjectLayerFilter(ezUInt32 uiCollisionLayer)
    : m_uiCollisionLayer(uiCollisionLayer)
  {
  }

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override;
};

class EZ_JOLTPLUGIN_DLL ezJoltObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  ezJoltObjectVsBroadPhaseLayerFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

class EZ_JOLTPLUGIN_DLL ezJoltObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
  ezJoltObjectLayerPairFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
};

class EZ_JOLTPLUGIN_DLL ezJoltBodyFilter final : public JPH::BodyFilter
{
public:
  ezUInt32 m_uiObjectFilterIDToIgnore = ezInvalidIndex - 1;

  ezJoltBodyFilter(ezUInt32 uiBodyFilterIdToIgnore = ezInvalidIndex - 1)
    : m_uiObjectFilterIDToIgnore(uiBodyFilterIdToIgnore)
  {
  }

  void ClearFilter()
  {
    m_uiObjectFilterIDToIgnore = ezInvalidIndex - 1;
  }

  virtual bool ShouldCollideLocked(const JPH::Body& body) const override
  {
    return body.GetCollisionGroup().GetGroupID() != m_uiObjectFilterIDToIgnore;
  }
};
