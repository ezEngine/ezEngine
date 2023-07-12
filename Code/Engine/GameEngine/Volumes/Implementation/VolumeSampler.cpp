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

  if (interpolationDuration.IsPositive())
  {
    // Reach 90% of target value after interpolation duration:
    // Lerp factor for exponential moving average:
    // y = 1-(1-f)^t
    // solve for f with y = 0.9:
    // f = 1 - 10^(-1 / t)
    value.m_fInterpolationFactor = 1.0 - ezMath::Pow(10.0, -1.0 / interpolationDuration.GetSeconds());
  }
  else
  {
    value.m_fInterpolationFactor = -1.0;
  }
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
  struct ComponentInfo
  {
    const ezVolumeComponent* m_pComponent = nullptr;
    ezUInt32 m_uiSortingKey = 0;
    float m_fAlpha = 0.0f;

    bool operator<(const ComponentInfo& other) const
    {
      return m_uiSortingKey < other.m_uiSortingKey;
    }
  };

  m_TargetValues.Clear();

  auto vPos = ezSimdConversion::ToVec3(vGlobalPosition);
  ezBoundingSphere sphere(vGlobalPosition, 0.01f);

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = s_VolumeCategory.GetBitmask();

  ezHybridArray<ComponentInfo, 16> componentInfos;
  world.GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, [&](ezGameObject* pObject)
    {
      ezVolumeComponent* pComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pComponent))
      {
        ComponentInfo info;
        info.m_pComponent = pComponent;

        ezSimdTransform scaledTransform = pComponent->GetOwner()->GetGlobalTransformSimd();

        if (auto pBoxComponent = ezDynamicCast<const ezVolumeBoxComponent*>(pComponent))
        {
          scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(ezSimdConversion::ToVec3(pBoxComponent->GetExtents())) * 0.5f;

          ezSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const ezSimdVec4f absLocalPos = globalToLocalTransform.TransformPosition(vPos).Abs();
          if ((absLocalPos <= ezSimdVec4f(1.0f)).AllSet<3>())
          {
            ezSimdVec4f vAlpha = (ezSimdVec4f(1.0f) - absLocalPos).CompDiv(ezSimdConversion::ToVec3(pBoxComponent->GetFalloff().CompMax(ezVec3(0.0001f))));
            vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::ZeroVector());
            info.m_fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
          }
        }
        else if (auto pSphereComponent = ezDynamicCast<const ezVolumeSphereComponent*>(pComponent))
        {
          scaledTransform.m_Scale *= pSphereComponent->GetRadius();

          ezSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const ezSimdVec4f localPos = globalToLocalTransform.TransformPosition(vPos);
          const float distSquared = localPos.GetLengthSquared<3>();
          if (distSquared <= 1.0f)
          {
            info.m_fAlpha = ezMath::Saturate((1.0f - ezMath::Sqrt(distSquared)) / pSphereComponent->GetFalloff());
          }
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }

        if (info.m_fAlpha > 0.0f)
        {
          info.m_uiSortingKey = ComputeSortingKey(pComponent->GetSortOrder(), scaledTransform.GetMaxScale());

          componentInfos.PushBack(info);
        }
      }

      return ezVisitorExecution::Continue; });

  // Sort
  {
    componentInfos.Sort();
  }

  for (auto& info : componentInfos)
  {
    ezResourceLock<ezBlackboardTemplateResource> blackboardTemplate(info.m_pComponent->GetTemplate(), ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (blackboardTemplate.GetAcquireResult() != ezResourceAcquireResult::Final)
      continue;

    auto& desc = blackboardTemplate->GetDescriptor();
    for (auto& entry : desc.m_Entries)
    {
      auto pValue = m_Values.GetValue(entry.m_sName);
      if (pValue == nullptr)
        continue;

      ezVariant currentValue;
      if (m_TargetValues.TryGetValue(entry.m_sName, currentValue) == false)
      {
        currentValue = pValue->m_DefaultValue;
      }

      ezEnum<ezVariantType> targetType = currentValue.GetType();

      ezResult conversionStatus = EZ_SUCCESS;
      ezVariant targetValue = entry.m_InitialValue.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        ezLog::Error("VolumeSampler: Can't convert template value '{}' to '{}'.", entry.m_sName, targetType);
        continue;
      }      

      m_TargetValues[entry.m_sName] = ezMath::Lerp(currentValue, targetValue, double(info.m_fAlpha));
    }
  }

  for (auto& it : m_Values)
  {
    auto& value = it.Value();

    ezVariant targetValue;
    if (m_TargetValues.TryGetValue(it.Key(), targetValue) == false)
    {
      targetValue = value.m_DefaultValue;
    }

    if (value.m_fInterpolationFactor > 0.0)
    {
      double f = 1.0 - ezMath::Pow(1.0 - value.m_fInterpolationFactor, deltaTime.GetSeconds());
      value.m_CurrentValue = ezMath::Lerp(value.m_CurrentValue, targetValue, f);
    }
    else
    {
      value.m_CurrentValue = targetValue;
    }
  }
}

// static
ezUInt32 ezVolumeSampler::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  ezUInt32 uiSortingKey = (ezUInt32)(ezMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey = (uiSortingKey << 16) | (0xFFFF - ((ezUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}
