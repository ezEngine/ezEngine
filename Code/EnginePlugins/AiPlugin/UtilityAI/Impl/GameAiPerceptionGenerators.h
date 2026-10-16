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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Tracks a single target from ezAiSensorSpatial sensors and remembers its last known position for a while.
///
/// When the owner receives ezMsgDamage while the target isn't visible, the object with m_sDamageSourceGlobalKey
/// is assumed to be the attacker and its current position becomes the last known position (this is cheating).
class EZ_AIPLUGIN_DLL ezAiPerceptionGenTarget : public ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenTarget();
  ~ezAiPerceptionGenTarget();

  virtual ezStringView GetPerceptionType() override { return "ezAiPerceptionTarget"_ezsv; }
  virtual void UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) override;
  virtual void HandleMessage(ezGameObject& owner, const ezMessage& msg) override;

  /// Names of ezAiSensorSpatial sensors in the sensor manager, checked in order.
  ezHybridArray<ezString, 2> m_SensorNames;
  ezString m_sDamageSourceGlobalKey;
  ezTime m_MemoryDuration = ezTime::Seconds(30);

private:
  bool m_bHasTarget = false;
  bool m_bDamageReceived = false;
  ezAiPerceptionTarget m_Target;
};
