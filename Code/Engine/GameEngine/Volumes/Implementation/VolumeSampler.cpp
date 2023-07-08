#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>

extern ezSpatialData::Category s_VolumeCategory;

ezVolumeSampler::ezVolumeSampler() = default;
ezVolumeSampler::~ezVolumeSampler() = default;

void ezVolumeSampler::RegisterValue(ezHashedString sName, ezVariant defaultValue, ezTime interpolationDuration /*= ezTime::Zero()*/)
{
  auto& value = m_CurrentValues[sName];
  value.m_Value = defaultValue;
  value.m_InterpolationDuration = interpolationDuration;
}

void ezVolumeSampler::DeregisterValue(ezHashedString sName)
{
  m_CurrentValues.Remove(sName);
}

void ezVolumeSampler::DeregisterAllValues()
{
  m_CurrentValues.Clear();
}

void ezVolumeSampler::SampleAtPosition(ezWorld& world, const ezVec3& vGlobalPosition, ezTime deltaTime)
{
  ezBoundingSphere sphere(vGlobalPosition, 0.01f);

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = s_VolumeCategory.GetBitmask();

  ezHybridArray<const ezVolumeComponent*, 16> volumeComponents;
  world.GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, [&](ezGameObject* pObject)
    {
      ezVolumeComponent* pComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pComponent))
      {
        volumeComponents.PushBack(pComponent);
      }

      return ezVisitorExecution::Continue;
    });


}
