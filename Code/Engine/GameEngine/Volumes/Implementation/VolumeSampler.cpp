#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>

ezVolumeSampler::ezVolumeSampler() = default;
ezVolumeSampler::~ezVolumeSampler() = default;

void ezVolumeSampler::RegisterValue(ezHashedString sName, ezVariant defaultValue, ezTime interpolationDuration /*= ezTime::MakeZero()*/)
{
  auto& value = m_Values[sName];
  value.m_DefaultValue = defaultValue;
  value.m_TargetValue = defaultValue;
  value.m_CurrentValue = defaultValue;

  if (interpolationDuration.IsPositive())
  {
    // Reach 90% of target value after interpolation duration:
    // Lerp factor for exponential moving average:
    // y = 1-f^t
    // solve for f with y = 0.9:
    // f = 10^(-1 / t)
    value.m_fInterpolationFactor = ezMath::Pow(10.0, -1.0 / interpolationDuration.GetSeconds());
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

void ezVolumeSampler::SampleAtPosition(const ezWorld& world, ezSpatialData::Category spatialCategory, const ezVec3& vGlobalPosition, ezTime deltaTime)
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

  auto vPos = ezSimdConversion::ToVec3(vGlobalPosition);
  ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(vGlobalPosition, 0.01f);

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();

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
            ezSimdVec4f vAlpha = (ezSimdVec4f(1.0f) - absLocalPos).CompDiv(ezSimdConversion::ToVec3(pBoxComponent->GetFalloff()));
            vAlpha = vAlpha.CompMin(ezSimdVec4f(1.0f)).CompMax(ezSimdVec4f::MakeZero());
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

  for (auto& it : m_Values)
  {
    auto& sName = it.Key();
    auto& value = it.Value();

    value.m_TargetValue = value.m_DefaultValue;

    for (auto& info : componentInfos)
    {
      ezVariant volumeValue = info.m_pComponent->GetValue(sName);
      if (volumeValue.IsValid() == false)
        continue;

      ezResult conversionStatus = EZ_SUCCESS;
      ezEnum<ezVariantType> targetType = value.m_TargetValue.GetType();
      ezVariant newTargetValue = volumeValue.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        ezLog::Error("VolumeSampler: Can't convert volume value '{}' to '{}'.", sName, targetType);
        continue;
      }

      value.m_TargetValue = ezMath::Lerp(value.m_TargetValue, newTargetValue, double(info.m_fAlpha));
    }

    if (value.m_fInterpolationFactor > 0.0)
    {
      double f = 1.0 - ezMath::Pow(value.m_fInterpolationFactor, deltaTime.GetSeconds());
      value.m_CurrentValue = ezMath::Lerp(value.m_CurrentValue, value.m_TargetValue, f);
    }
    else
    {
      value.m_CurrentValue = value.m_TargetValue;
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
