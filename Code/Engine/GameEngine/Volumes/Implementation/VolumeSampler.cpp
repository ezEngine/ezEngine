#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>

extern ezSpatialData::Category s_VolumeCategory;

ezVolumeSampler::ezVolumeSampler() = default;
ezVolumeSampler::~ezVolumeSampler() = default;

void ezVolumeSampler::RegisterValue(ezHashedString sName, ezVariant defaultValue, ezTime interpolationDuration /*= ezTime::Zero()*/)
{
  auto& value = m_Values[sName];
  value.m_DefaultValue = defaultValue;
  value.m_CurrentValue = defaultValue;
  value.m_InterpolationDuration = interpolationDuration;
}

void ezVolumeSampler::DeregisterValue(ezHashedString sName)
{
  m_Values.Remove(sName);
}

void ezVolumeSampler::DeregisterAllValues()
{
  m_Values.Clear();
}

void ezVolumeSampler::SampleAtPosition(ezWorld& world, const ezVec3& vGlobalPosition, ezTime deltaTime)
{
  m_TargetValues.Clear();

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

  // TODO: sorting
  
  auto vPos = ezSimdConversion::ToVec3(vGlobalPosition);

  for (auto pVolumeComponent : volumeComponents)
  {
    ezSimdTransform scaledTransform = pVolumeComponent->GetOwner()->GetGlobalTransformSimd();
    float fAlpha = 0.0f;

    // TODO:  sphere
    if (auto pBoxVolume = ezDynamicCast<const ezVolumeBoxComponent*>(pVolumeComponent))
    {      
      scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(pBoxVolume->GetExtents())) * 0.5f;

      ezSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
      const ezSimdVec4f absLocalPos = globalToLocalTransform.TransformPosition(vPos).Abs();
      if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
      {
        ezSimdVec4f vAlpha = (ezSimdVec4f(1.0f) - absLocalPos).CompDiv(ezSimdConversion::ToVec3(pBoxVolume->GetFalloff().CompMax(ezVec3(0.0001f))));
        vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::ZeroVector());
        fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
      }
    }

    if (fAlpha <= 0.0f)
      continue;

    ezResourceLock<ezBlackboardTemplateResource> blackboardTemplate(pVolumeComponent->GetTemplate(), ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (blackboardTemplate.GetAcquireResult() != ezResourceAcquireResult::Final)
      continue;

    auto& desc = blackboardTemplate->GetDescriptor();
    for (auto& entry : desc.m_Entries)
    {
      auto pCurrentValue = m_Values.GetValue(entry.m_sName);
      if (pCurrentValue == nullptr)
        continue;

      // TODO: interpolate
      m_TargetValues[entry.m_sName] = entry.m_InitialValue;
    }
  }

  for (auto& it : m_Values)
  {
    ezVariant targetValue;
    if (m_TargetValues.TryGetValue(it.Key(), targetValue) == false)
    {
      targetValue = it.Value().m_DefaultValue;
    }

    // TODO: fade over time
    it.Value().m_CurrentValue = targetValue;
  }
}
