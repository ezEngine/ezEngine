#pragma once

#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

class ezWorld;

/// \brief A volume sampler is used to sample the registered values from volumes at a given position. It also takes care of interpolation over time of those values.
class EZ_GAMEENGINE_DLL ezVolumeSampler
{
public:
  ezVolumeSampler();
  ~ezVolumeSampler();

  void RegisterValue(ezHashedString sName, ezVariant defaultValue, ezTime interpolationDuration = ezTime::Zero());
  void DeregisterValue(ezHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(ezWorld& world, ezSpatialData::Category spatialCategory, const ezVec3& vGlobalPosition, ezTime deltaTime);

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
    ezVariant m_TargetValue;
    ezVariant m_CurrentValue;
    double m_fInterpolationFactor = -1.0;
  };

  ezHashTable<ezHashedString, Value> m_Values;
};
