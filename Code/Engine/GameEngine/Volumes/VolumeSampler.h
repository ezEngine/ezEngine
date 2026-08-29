#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/SpatialData.h>
#include <Foundation/Types/Variant.h>

class ezBlackboard;

/// A volume sampler is used to sample the registered values from volumes at a given position. It also takes care of interpolation over time of those values.
class EZ_GAMEENGINE_DLL ezVolumeSampler
{
public:
  ezVolumeSampler();
  ~ezVolumeSampler();

  void RegisterValue(ezHashedString sName, ezVariant defaultValue, ezTime interpolationDuration = ezTime::MakeZero());
  void DeregisterValue(ezHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(const ezWorld& world, ezSpatialData::Category spatialCategory, const ezVec3& vGlobalPosition, ezTime deltaTime, ezBlackboard* pTargetBlackboard = nullptr);

  ezVariant GetValue(ezTempHashedString sName) const
  {
    if (const Value* pValue = m_Values.GetValue(sName))
    {
      return pValue->m_CurrentValue;
    }

    return ezVariant();
  }

  static ezUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

private:
  struct Value
  {
    ezVariant m_DefaultValue;
    ezVariant m_CurrentValue;
    double m_fInterpolationFactor = -1.0;

    ezHashedString m_sStrengthName;
    float m_fCurrentStrength = 0.0f;
  };

  ezHashTable<ezHashedString, Value> m_Values;
};
