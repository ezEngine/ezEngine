#pragma once

#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

class ezWorld;

class EZ_GAMEENGINE_DLL ezVolumeSampler
{
public:
  ezVolumeSampler();
  ~ezVolumeSampler();

  void RegisterValue(ezHashedString sName, ezVariant defaultValue, ezTime interpolationDuration = ezTime::Zero());
  void DeregisterValue(ezHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(ezWorld& world, const ezVec3& vGlobalPosition, ezTime deltaTime);

  ezVariant GetValue(ezTempHashedString sName) const
  {
    if (const Value* pValue = m_CurrentValues.GetValue(sName))
    {
      return pValue->m_Value;
    }

    return ezVariant();
  }

private:
  struct Value
  {
    ezVariant m_Value;
    ezTime m_InterpolationDuration;
  };

  ezHashTable<ezHashedString, Value> m_CurrentValues;
};
