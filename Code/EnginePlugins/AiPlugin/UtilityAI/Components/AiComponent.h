#pragma once

#include <Core/World/World.h>
#include <Foundation/Containers/StaticRingBuffer.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiBehaviorManager.h>
#include <AiPlugin/UtilityAI/Framework/AiConsiderationManager.h>
#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiDecisionRecorder.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>
#include <AiPlugin/UtilityAI/Resources/AiControllerResource.h>

struct ezMsgDamage;

/// Updates all ezAiComponent instances. While the CVar 'Ai.DebugView' is on, shows an ImGui window for the agent closest to the screen center.
///
/// The selection stays on an agent until another one is closer to the screen center. F7 pins / unpins the selection.
///
/// While the CVar 'Ai.Recording' is on (default), every agent records its last decisions (see ezAiDecisionRecorder).
/// Setting the CVar 'Ai.DumpRecording' or pressing F6 writes the recordings of all agents to ':appdata/AiRecordings/' as JSON. The log says where.
class EZ_AIPLUGIN_DLL ezAiComponentManager : public ezComponentManager<class ezAiComponent, ezBlockStorageType::FreeList>
{
public:
  ezAiComponentManager(ezWorld* pWorld);
  ~ezAiComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
  void UpdateDebugView();
  void DumpRecordings();

  ezComponentHandle m_hDebugViewAgent;
  bool m_bDebugViewPinned = false; ///< Toggled with F7, keeps the selected agent.
};

/// Runs the utility AI for one NPC.
///
/// Every frame, the active behavior gets ContinueBehavior() called and the action queue is executed.
/// Every m_DecisionInterval (and right away when the active behavior ends), a decision update runs:
///  1. Behaviors that are not on cooldown (and their considerations) flag the perceptions they need, which flag the sensors they need.
///  2. Only the flagged sensors and perception generators are updated.
///  3. The available behaviors are scored, using their considerations. The best one replaces the active behavior if it scores more than 0.1 higher.
///     The active behavior is re-scored as well (unless on cooldown), so it can lose when its own score drops.
/// No decision update happens while the active behavior disallows switching (see ezAiBehavior::ContinuationState).
///
/// The sensors, perception generators and behaviors come from the controller resource (an AI Controller asset).
/// The resource is loaded synchronously when the simulation starts. Without a resource the component does nothing.
///
/// ezMsgDamage is forwarded to the perception generators.
class EZ_AIPLUGIN_DLL ezAiComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiComponent, ezComponent, ezAiComponentManager);
  friend class ezAiComponentManager;

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

  /// Draws the active behavior, current action and all behavior scores above the NPC and logs behavior switches.
  bool m_bDebugInfo = false;                        // [ property ]
  ezAiControllerResourceHandle m_hController;       // [ property ]
  ezTime m_DecisionInterval = ezTime::Seconds(0.5); // [ property ]

protected:
  /// Releases the instance data and the controller desc. Called on deactivation.
  void ClearController();
  void DrawDebugInfo();
  void SetActiveBehavior(const ezAiContext& ctx, ezUInt32 uiBehaviorIndex, const ezAiPerception* pPerception, float fScore);

  /// Shown in the debug view.
  struct BehaviorSwitch
  {
    ezTime m_Time;
    ezUInt32 m_uiFromBehavior = ezInvalidIndex;
    ezUInt32 m_uiToBehavior = ezInvalidIndex; ///< ezInvalidIndex if the behavior ended and nothing else was chosen yet.
    float m_fScore = 0.0f;
  };

  ezSharedPtr<ezAiControllerDesc> m_pControllerDesc;
  ezBlob m_InstanceData; ///< Instance data of all sensors, perception generators, behaviors and considerations, see ezAiControllerDesc.

  ezAiActionQueue m_ActionQueue;
  ezTime m_LastAiUpdate;

  ezAiSensorManager m_SensorManager;
  ezAiPerceptionManager m_PerceptionManager;
  ezAiConsiderationManager m_ConsiderationManager;
  ezAiBehaviorManager m_BehaviorManager;
  float m_fLastScore = 0.0f; ///< Latest score of the active behavior, 0 if there is none.
  ezStaticRingBuffer<BehaviorSwitch, 8> m_RecentSwitches;
  ezAiDecisionRecorder m_Recorder;
};
