#include <JoltPlugin/JoltPluginPCH.h>

#include <GameEngine/Physics/CollisionFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>

namespace ezJoltCollisionFiltering
{
  ezCollisionFilterConfig s_CollisionFilterConfig;

  JPH::ObjectLayer ConstructObjectLayer(ezUInt8 uiCollisionGroup, ezJoltBroadphaseLayer broadphase)
  {
    return static_cast<JPH::ObjectLayer>(static_cast<ezUInt16>(broadphase) << 8 | static_cast<ezUInt16>(uiCollisionGroup));
  }

  void LoadCollisionFilters()
  {
    EZ_LOG_BLOCK("ezJoltCore::LoadCollisionFilters");

    if (s_CollisionFilterConfig.Load().Failed())
    {
      ezLog::Info("Collision filter config file could not be found ('{}'). Using default values.", ezCollisionFilterConfig::s_sConfigFile);

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
    // this mapping defines which types of objects can generally collide with each other
    // if a flag is not included here, those types will never collide, no matter what their collision group is and other filter settings are
    // note that this is only used for the simulation, raycasts and shape queries can use their own mapping

    switch (broadphase)
    {
      case ezJoltBroadphaseLayer::Static:
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Character) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Ragdoll) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Rope) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Cloth);

      case ezJoltBroadphaseLayer::Dynamic:
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Static) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Trigger) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Character) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Ragdoll) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Rope) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Cloth);

      case ezJoltBroadphaseLayer::Query:
        // query shapes never interact with anything in the simulation
        return 0;

      case ezJoltBroadphaseLayer::Trigger:
        // triggers specifically exclude detail objects such as ropes, ragdolls and queries (also used for hitboxes) for performance reasons
        // if necessary, these shapes can still be found with overlap queries
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Character);

      case ezJoltBroadphaseLayer::Character:
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Static) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Trigger) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Character) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Cloth);

      case ezJoltBroadphaseLayer::Ragdoll:
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Static) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Ragdoll) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Rope) /*| EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Cloth)*/; // Ragdolls currently crash Jolt, due to continuous collision detection

      case ezJoltBroadphaseLayer::Rope:
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Static) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Ragdoll) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Rope);

      case ezJoltBroadphaseLayer::Cloth:
        return EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Static) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic) | EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Character) /*| EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Ragdoll)*/; // Ragdolls currently crash Jolt, due to continuous collision detection

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return 0;
  };

} // namespace ezJoltCollisionFiltering

ezUInt32 ezJoltObjectToBroadphaseLayer::GetNumBroadPhaseLayers() const
{
  return (ezUInt32)ezJoltBroadphaseLayer::ENUM_COUNT;
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

    case Query:
      return "QueryShapes";

    case Trigger:
      return "Trigger";

    case Character:
      return "Character";

    case Ragdoll:
      return "Ragdoll";

    case Rope:
      return "Rope";

    case Cloth:
      return "Cloth";

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
#endif

// if any of these asserts fails, ezPhysicsShapeType and ezJoltBroadphaseLayer are out of sync
static_assert(ezPhysicsShapeType::Static == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Static));
static_assert(ezPhysicsShapeType::Dynamic == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Dynamic));
static_assert(ezPhysicsShapeType::Query == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Query));
static_assert(ezPhysicsShapeType::Trigger == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Trigger));
static_assert(ezPhysicsShapeType::Character == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Character));
static_assert(ezPhysicsShapeType::Ragdoll == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Ragdoll));
static_assert(ezPhysicsShapeType::Rope == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Rope));
static_assert(ezPhysicsShapeType::Cloth == EZ_BIT((ezUInt32)ezJoltBroadphaseLayer::Cloth));
static_assert(ezPhysicsShapeType::Count == (ezUInt32)ezJoltBroadphaseLayer::ENUM_COUNT);

bool ezJoltObjectLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer) const
{
  return ezJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(m_uiCollisionLayer, static_cast<ezUInt32>(inLayer) & 0xFF);
}

bool ezJoltObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
{
  const ezUInt32 uiMask1 = static_cast<ezUInt32>(EZ_BIT(inLayer1 >> 8));
  const ezUInt32 uiMask2 = ezJoltCollisionFiltering::GetBroadphaseCollisionMask(static_cast<ezJoltBroadphaseLayer>((ezUInt8)inLayer2));

  return (uiMask1 & uiMask2) != 0;
}

bool ezJoltObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
{
  return ezJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(static_cast<ezUInt32>(inObject1) & 0xFF, static_cast<ezUInt32>(inObject2) & 0xFF);
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltCollisionFiltering);
