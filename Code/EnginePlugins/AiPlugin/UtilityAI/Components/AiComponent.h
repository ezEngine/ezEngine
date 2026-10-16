#pragma once

#include <Core/World/World.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiBehaviorManager.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

struct ezMsgDamage;

/// Selects which hard-coded set of sensors, perceptions and behaviors an ezAiComponent uses.
///
/// Temporary, until behaviors can be configured as data.
struct ezAiBehaviorSet
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Prototype, ///< The original test setup (points of interest, checkpoints, quips).
    AlBot,     ///< Re-implementation of the Testing Chambers "Al" bot state machine.

    Default = Prototype
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiBehaviorSet);

using ezAiComponentManager = ezComponentManagerSimple<class ezAiComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

class EZ_AIPLUGIN_DLL ezAiComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiComponent, ezComponent, ezAiComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAiComponent

  void Update();
  void OnMsgDamage(ezMsgDamage& ref_msg); // [ msg handler ]

public:
  ezAiComponent();
  ~ezAiComponent();

  bool m_bDebugInfo = false;                        // [ property ]
  ezEnum<ezAiBehaviorSet> m_BehaviorSet;            // [ property ]
  ezTime m_DecisionInterval = ezTime::Seconds(0.5); // [ property ]

protected:
  void SetupPrototype();
  void SetupAlBot();
  void DrawDebugInfo();

  ezAiActionQueue m_ActionQueue;
  ezTime m_LastAiUpdate;

  ezAiSensorManager m_SensorManager;
  ezAiPerceptionManager m_PerceptionManager;
  ezAiBehaviorManager m_BehaviorManager;
  float m_fLastScore = 0.0f;
};
