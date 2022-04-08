#include <JoltPlugin/JoltPluginPCH.h>

#include <GameEngine/Physics/CollisionFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>

namespace ezJoltCollisionFiltering
{
  ezCollisionFilterConfig s_CollisionFilterConfig;

  bool BroadphaseFilter(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2)
  {
    const ezUInt32 uiMask1 = EZ_BIT(inLayer1 >> 8);
    const ezUInt32 uiMask2 = GetBroadphaseCollisionMask(static_cast<ezJoltBroadphaseLayer>((ezUInt8)inLayer2));

    return (uiMask1 & uiMask2) != 0;
  }

  bool ObjectLayerFilter(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2)
  {
    return s_CollisionFilterConfig.IsCollisionEnabled(static_cast<ezUInt32>(inObject1) & 0xFF, static_cast<ezUInt32>(inObject2) & 0xFF);
  };

  JPH::ObjectLayer ConstructObjectLayer(ezUInt8 uiCollisionGroup, ezJoltBroadphaseLayer broadphase)
  {
    return static_cast<JPH::ObjectLayer>(static_cast<ezUInt16>(broadphase) << 8 | static_cast<ezUInt16>(uiCollisionGroup));
  }

  void LoadCollisionFilters()
  {
    EZ_LOG_BLOCK("ezJoltCore::LoadCollisionFilters");

    if (s_CollisionFilterConfig.Load("RuntimeConfigs/CollisionLayers.cfg").Failed())
    {
      ezLog::Info("Collision filter config file could not be found ('RuntimeConfigs/CollisionLayers.cfg'). Using default values.");

      // setup some default config

      s_CollisionFilterConfig.SetGroupName(0, "Default");
      s_CollisionFilterConfig.EnableCollision(0, 0);
    }
  }

  ezCollisionFilterConfig& GetCollisionFilterConfig()
  {
    return s_CollisionFilterConfig;
  }

  ezUInt32 GetBroadphaseCollisionMask(ezJoltBroadphaseLayer broadphase)
  {
    switch (broadphase)
    {
      case Static:
        return EZ_BIT(ezJoltBroadphaseLayer::Dynamic) | EZ_BIT(ezJoltBroadphaseLayer::Detail) | EZ_BIT(ezJoltBroadphaseLayer::Character);

      case Dynamic:
        return EZ_BIT(ezJoltBroadphaseLayer::Static) | EZ_BIT(ezJoltBroadphaseLayer::Dynamic) | EZ_BIT(ezJoltBroadphaseLayer::Trigger) | EZ_BIT(ezJoltBroadphaseLayer::Detail) | EZ_BIT(ezJoltBroadphaseLayer::Character);

      case Trigger:
        return EZ_BIT(ezJoltBroadphaseLayer::Dynamic) | EZ_BIT(ezJoltBroadphaseLayer::Character);

      case QueryShape:
        return 0;

      case Detail:
        return EZ_BIT(ezJoltBroadphaseLayer::Static) | EZ_BIT(ezJoltBroadphaseLayer::Dynamic);

      case Character:
        return EZ_BIT(ezJoltBroadphaseLayer::Static) | EZ_BIT(ezJoltBroadphaseLayer::Dynamic) | EZ_BIT(ezJoltBroadphaseLayer::Trigger) | EZ_BIT(ezJoltBroadphaseLayer::Character);
        break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return 0;
  };

} // namespace ezJoltCollisionFiltering

ezUInt32 ezJoltObjectToBroadphaseLayer::GetNumBroadPhaseLayers() const
{
  return ezJoltBroadphaseLayer::ENUM_COUNT;
}

JPH::BroadPhaseLayer ezJoltObjectToBroadphaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
{
  return JPH::BroadPhaseLayer(inLayer >> 8);
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* ezJoltObjectToBroadphaseLayer::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
{
  switch (inLayer)
  {
    case Static:
      return "Static";

    case Dynamic:
      return "Dynamic";

    case Trigger:
      return "Trigger";

    case QueryShape:
      return "QueryShapes";

    case Detail:
      return "Debris";

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
#endif

ezJoltBroadPhaseLayerFilter::ezJoltBroadPhaseLayerFilter(ezBitflags<ezPhysicsShapeType> shapeTypes)
{
  if (shapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    m_uiCollisionMask |= EZ_BIT(ezJoltBroadphaseLayer::Static);
  }

  if (shapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    m_uiCollisionMask |= EZ_BIT(ezJoltBroadphaseLayer::Dynamic);
    m_uiCollisionMask |= EZ_BIT(ezJoltBroadphaseLayer::Character);
    m_uiCollisionMask |= EZ_BIT(ezJoltBroadphaseLayer::Detail);
  }

  if (shapeTypes.IsSet(ezPhysicsShapeType::Query))
  {
    m_uiCollisionMask |= EZ_BIT(ezJoltBroadphaseLayer::QueryShape);
  }
}

bool ezJoltObjectLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer) const
{
  return ezJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(m_uiCollisionLayer, static_cast<ezUInt32>(inLayer) & 0xFF);
}
