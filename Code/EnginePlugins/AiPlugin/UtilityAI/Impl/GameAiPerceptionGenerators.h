#pragma once

#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <Core/World/SpatialData.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

/// Shared implementation for generators whose instance data is just an array of perceptions.
template <typename PERCEPTION>
class ezAiPerceptionGenArray : public ezAiPerceptionGenerator
{
public:
  virtual bool HasPerceptions(const void* pInstanceData) const override
  {
    return !static_cast<const InstanceData*>(pInstanceData)->m_Perceptions.IsEmpty();
  }

  virtual void GetPerceptions(const void* pInstanceData, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override
  {
    const auto& perceptions = static_cast<const InstanceData*>(pInstanceData)->m_Perceptions;

    out_Perceptions.Reserve(out_Perceptions.GetCount() + perceptions.GetCount());
    for (const auto& perception : perceptions)
    {
      out_Perceptions.PushBack(&perception);
    }
  }

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override
  {
    out_desc.FillFromType<InstanceData>();
    return true;
  }

protected:
  struct InstanceData
  {
    ezDynamicArray<PERCEPTION> m_Perceptions;
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// One ezAiPerceptionPOI per object that the connected ezAiSensorSpatial sensors detected.
class EZ_AIPLUGIN_DLL ezAiPerceptionGenPOI : public ezAiPerceptionGenArray<ezAiPerceptionPOI>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionGenPOI, ezAiPerceptionGenerator);

public:
  ezAiPerceptionGenPOI();
  ~ezAiPerceptionGenPOI();

  virtual void UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const override;

  ezAiInputPin m_Sensors; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Seven fixed positions in front of, beside and behind the NPC (3m forward, 5m sideways). Doesn't check whether they are reachable.
class EZ_AIPLUGIN_DLL ezAiPerceptionGenWander : public ezAiPerceptionGenArray<ezAiPerceptionWander>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionGenWander, ezAiPerceptionGenerator);

public:
  ezAiPerceptionGenWander();
  ~ezAiPerceptionGenWander();

  virtual void UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const override {}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// All objects in the 'Checkpoint' spatial category within m_fRadius. Uses a spatial query directly, no sensor.
class EZ_AIPLUGIN_DLL ezAiPerceptionGenCheckpoint : public ezAiPerceptionGenArray<ezAiPerceptionCheckpoint>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionGenCheckpoint, ezAiPerceptionGenerator);

public:
  ezAiPerceptionGenCheckpoint();
  ~ezAiPerceptionGenCheckpoint();

  virtual void UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const override {}

  float m_fRadius = 100.0f; // [ property ]

private:
  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// One ezAiPerceptionTarget per object that the connected ezAiSensorSpatial sensors detect, remembered for a while once it isn't detected anymore.
///
/// While an object is detected, its data is refreshed on every update. Once it isn't, its perception keeps the last known data
/// until m_MemoryDuration has passed, or until the object is deleted or deactivated.
/// Which objects count as targets is decided by the sensor components (spatial category, include / exclude tags).
/// The perceptions keep their order, new targets are appended.
class EZ_AIPLUGIN_DLL ezAiPerceptionGenTarget : public ezAiPerceptionGenArray<ezAiPerceptionTarget>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionGenTarget, ezAiPerceptionGenerator);

public:
  ezAiPerceptionGenTarget();
  ~ezAiPerceptionGenTarget();

  virtual void UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const override;

  ezAiInputPin m_Sensors; // [ property ]

  /// How long a target that isn't detected anymore is remembered, measured from the last update in which it was detected.
  ezTime m_MemoryDuration = ezTime::Seconds(30); // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// One ezAiPerceptionHint per ezAiMarkerComponent that the connected 'Sensors' detect. Objects without such a marker are ignored.
///
/// Hints whose source (see ezAiMarkerComponent::GetSourceObject()) is detected by one of the 'DirectSensors' are dropped,
/// e.g. the noise of a shooter that the NPC sees anyway. Of the remaining hints, only the ones with the highest priority are kept.
/// Nothing is remembered, every update starts from scratch.
class EZ_AIPLUGIN_DLL ezAiPerceptionGenHint : public ezAiPerceptionGenArray<ezAiPerceptionHint>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionGenHint, ezAiPerceptionGenerator);

public:
  ezAiPerceptionGenHint();
  ~ezAiPerceptionGenHint();

  virtual void UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const override;

  /// Sensors that detect the hint markers, e.g. a 'hearing' sphere sensor for the 'Noise' spatial category.
  ezAiInputPin m_Sensors; // [ property ]

  /// Sensors that detect the sources directly, e.g. the vision sensor that sees the shooter.
  ezAiInputPin m_DirectSensors; // [ property ]
};
