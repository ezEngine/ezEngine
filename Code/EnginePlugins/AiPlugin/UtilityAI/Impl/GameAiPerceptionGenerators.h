#pragma once

#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiPerceptionGenPOI : public ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenPOI();
  ~ezAiPerceptionGenPOI();

  virtual ezStringView GetPerceptionType() override { return "ezAiPerceptionPOI"_ezsv; }
  virtual void UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) override;

private:
  ezDynamicArray<ezAiPerceptionPOI> m_Perceptions;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiPerceptionGenWander : public ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenWander();
  ~ezAiPerceptionGenWander();

  virtual ezStringView GetPerceptionType() override { return "ezAiPerceptionWander"_ezsv; }
  virtual void UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) override;

private:
  ezDynamicArray<ezAiPerceptionWander> m_Perceptions;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiPerceptionGenCheckpoint : public ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenCheckpoint();
  ~ezAiPerceptionGenCheckpoint();

  virtual ezStringView GetPerceptionType() override { return "ezAiPerceptionCheckpoint"_ezsv; }
  virtual void UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) override;

private:
  ezDynamicArray<ezAiPerceptionCheckpoint> m_Perceptions;
  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;
};
