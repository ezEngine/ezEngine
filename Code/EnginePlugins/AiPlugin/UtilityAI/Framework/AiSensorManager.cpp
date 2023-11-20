#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

ezAiSensorManager::ezAiSensorManager() = default;
ezAiSensorManager::~ezAiSensorManager() = default;

void ezAiSensorManager::AddSensor(ezStringView sName, ezUniquePtr<ezAiSensor>&& pSensor)
{
  auto& s = m_Sensors.ExpandAndGetRef();
  s.m_sName = sName;
  s.m_pSensor = std::move(pSensor);
}

void ezAiSensorManager::FlagAsNeeded(ezStringView sName)
{
  for (auto& s : m_Sensors)
  {
    if (s.m_sName == sName)
    {
      s.m_uiNeededInUpdate = m_uiUpdateCount;
      return;
    }
  }
}

void ezAiSensorManager::UpdateNeededSensors(ezGameObject& owner)
{
  for (auto& s : m_Sensors)
  {
    if (s.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      s.m_pSensor->UpdateSensor(owner);
    }
  }

  ++m_uiUpdateCount;
}

const ezAiSensor* ezAiSensorManager::GetSensor(ezStringView sName) const
{
  for (auto& s : m_Sensors)
  {
    if (s.m_sName == sName)
    {
      return s.m_pSensor.Borrow();
    }
  }

  return nullptr;
}
