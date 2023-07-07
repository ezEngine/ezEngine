#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeSampler.h>

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
  EZ_ASSERT_NOT_IMPLEMENTED;
}

