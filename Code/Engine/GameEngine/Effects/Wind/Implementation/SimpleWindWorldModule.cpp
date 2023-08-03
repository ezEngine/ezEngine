#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezSimpleWindWorldModule);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleWindWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSimpleWindWorldModule::ezSimpleWindWorldModule(ezWorld* pWorld)
  : ezWindWorldModuleInterface(pWorld)
{
  m_vFallbackWind.SetZero();
}

ezSimpleWindWorldModule::~ezSimpleWindWorldModule() = default;

ezVec3 ezSimpleWindWorldModule::GetWindAt(const ezVec3& vPosition) const
{
  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    ezHybridArray<ezGameObject*, 16> volumes;

    ezSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = ezWindVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(ezBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const ezSimdVec4f pos = ezSimdConversion::ToVec3(vPosition);
    ezSimdVec4f force = ezSimdVec4f::MakeZero();

    for (ezGameObject* pObj : volumes)
    {
      ezWindVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force += pVol->ComputeForceAtGlobalPosition(pos);
      }
    }

    return m_vFallbackWind + ezSimdConversion::ToVec3(force);
  }

  return m_vFallbackWind;
}

void ezSimpleWindWorldModule::SetFallbackWind(const ezVec3& vWind)
{
  m_vFallbackWind = vWind;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindWorldModule);
