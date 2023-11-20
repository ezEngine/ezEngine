#include <AiPlugin/AiPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

ezAiPerceptionManager::ezAiPerceptionManager() = default;
ezAiPerceptionManager::~ezAiPerceptionManager() = default;

void ezAiPerceptionManager::FlagPerceptionTypeAsNeeded(ezStringView sPerceptionType)
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_pPerceptionGenerator->GetPerceptionType() == sPerceptionType)
    {
      p.m_uiNeededInUpdate = m_uiUpdateCount;
    }
  }
}

void ezAiPerceptionManager::AddGenerator(ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator)
{
  auto& p = m_PerceptionGenerators.ExpandAndGetRef();
  p.m_pPerceptionGenerator = std::move(pGenerator);
}

void ezAiPerceptionManager::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      p.m_pPerceptionGenerator->FlagNeededSensors(ref_SensorManager);
    }
  }
}

void ezAiPerceptionManager::UpdateNeededPerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager)
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      p.m_pPerceptionGenerator->UpdatePerceptions(owner, ref_SensorManager);
    }
  }

  ++m_uiUpdateCount;
}

bool ezAiPerceptionManager::HasPerceptionsOfType(ezStringView sPerceptionType) const
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_pPerceptionGenerator->GetPerceptionType() == sPerceptionType && p.m_pPerceptionGenerator->HasPerceptions())
    {
      return true;
    }
  }

  return false;
}

void ezAiPerceptionManager::GetPerceptionsOfType(ezStringView sPerceptionType, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_pPerceptionGenerator->GetPerceptionType() == sPerceptionType)
    {
      p.m_pPerceptionGenerator->GetPerceptions(out_Perceptions);
    }
  }
}
