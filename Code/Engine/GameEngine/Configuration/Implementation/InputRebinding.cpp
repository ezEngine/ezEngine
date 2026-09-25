#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/Configuration/InputRebinding.h>

static_assert(ezInputRebinding::MaxSlots == ezGameAppInputConfig::MaxInputSlotAlternatives);

void ezInputRebinding::CollectActions(ezArrayPtr<const ezString> inputSets)
{
  CancelCapture();

  m_Bindings.Clear();
  m_bUnsavedChanges = false;
  m_bDefaultsRestored = false;

  ezTempHybridArray<ezString, 16> allSets;

  if (inputSets.IsEmpty())
  {
    ezInputManager::GetAllInputSets(allSets);

    for (ezUInt32 i = allSets.GetCount(); i > 0; --i)
    {
      if (IsEngineInputSet(allSets[i - 1]))
      {
        allSets.RemoveAtAndCopy(i - 1);
      }
    }

    // the input manager stores the sets in a map, sorting them keeps the order stable
    allSets.Sort();

    inputSets = allSets.GetArrayPtr();
  }

  ezTempHybridArray<ezString, 32> actions;

  for (const ezString& sSet : inputSets)
  {
    actions.Clear();
    ezInputManager::GetAllInputActions(sSet, actions);

    for (const ezString& sAction : actions)
    {
      auto& binding = m_Bindings.ExpandAndGetRef();
      binding.m_sInputSet = sSet;
      binding.m_sAction = sAction;
      binding.m_DefaultConfig = ezInputManager::GetInputActionConfig(sSet, sAction);
    }
  }

  // Actions that are in the project's file use the bindings from there, the current configuration may already contain the player's bindings.
  // Only the members that the file stores are taken, the rest (e.g. touch areas) stays as registered.
  ezFileReader file;
  if (file.Open(ezGameAppInputConfig::s_sConfigFile).Succeeded())
  {
    ezTempHybridArray<ezGameAppInputConfig, 32> projectConfigs;
    ezGameAppInputConfig::ReadFromDDL(file, projectConfigs);

    for (const ezGameAppInputConfig& projectCfg : projectConfigs)
    {
      for (auto& binding : m_Bindings)
      {
        if (binding.m_sInputSet != projectCfg.m_sInputSet || binding.m_sAction != projectCfg.m_sInputAction)
          continue;

        binding.m_DefaultConfig.m_bApplyTimeScaling = projectCfg.m_bApplyTimeScaling;

        for (ezUInt32 i = 0; i < MaxSlots; ++i)
        {
          binding.m_DefaultConfig.m_sInputSlotTrigger[i] = projectCfg.m_sInputSlotTrigger[i];
          binding.m_DefaultConfig.m_fInputSlotScale[i] = projectCfg.m_fInputSlotScale[i];
        }

        break;
      }
    }
  }
}

void ezInputRebinding::ApplyUserBindings()
{
  // the file only exists once something was rebound
  ezGameAppInputConfig::ApplyFile(ezGameAppInputConfig::s_sUserConfigFile).IgnoreResult();
}

bool ezInputRebinding::IsEngineInputSet(ezStringView sInputSet)
{
  // see ezGameApplication::ConfigureInputActions() and the console implementations
  return sInputSet == "GameApp" || sInputSet == "Console";
}

ezString ezInputRebinding::GetBoundSlot(ezUInt32 uiBinding, ezUInt32 uiSlot) const
{
  if (uiBinding >= m_Bindings.GetCount() || uiSlot >= MaxSlots)
    return {};

  const auto& binding = m_Bindings[uiBinding];
  return ezInputManager::GetInputActionConfig(binding.m_sInputSet, binding.m_sAction).m_sInputSlotTrigger[uiSlot];
}

void ezInputRebinding::SetBinding(ezUInt32 uiBinding, ezUInt32 uiSlot, ezStringView sInputSlot)
{
  if (uiBinding >= m_Bindings.GetCount() || uiSlot >= MaxSlots)
    return;

  const auto& binding = m_Bindings[uiBinding];

  ezInputActionConfig cfg = ezInputManager::GetInputActionConfig(binding.m_sInputSet, binding.m_sAction);
  cfg.m_sInputSlotTrigger[uiSlot] = sInputSlot;

  // binding a slot that the action already uses in another alternative moves it
  if (!sInputSlot.IsEmpty())
  {
    for (ezUInt32 i = 0; i < MaxSlots; ++i)
    {
      if (i != uiSlot && cfg.m_sInputSlotTrigger[i] == sInputSlot)
      {
        cfg.m_sInputSlotTrigger[i].Clear();
      }
    }
  }

  // removes the slot from the other actions of the set, so that it doesn't trigger two actions
  ezInputManager::SetInputActionConfig(binding.m_sInputSet, binding.m_sAction, cfg, true);

  m_bUnsavedChanges = true;
  m_bDefaultsRestored = false;
}

void ezInputRebinding::ClearBindings(ezUInt32 uiBinding)
{
  if (uiBinding >= m_Bindings.GetCount())
    return;

  const auto& binding = m_Bindings[uiBinding];

  ezInputActionConfig cfg = ezInputManager::GetInputActionConfig(binding.m_sInputSet, binding.m_sAction);

  for (ezUInt32 i = 0; i < MaxSlots; ++i)
  {
    cfg.m_sInputSlotTrigger[i].Clear();
  }

  ezInputManager::SetInputActionConfig(binding.m_sInputSet, binding.m_sAction, cfg, false);

  m_bUnsavedChanges = true;
  m_bDefaultsRestored = false;
}

void ezInputRebinding::RestoreDefault(ezUInt32 uiBinding)
{
  if (uiBinding >= m_Bindings.GetCount())
    return;

  const auto& binding = m_Bindings[uiBinding];

  // removes the default slots from other actions that the player may have bound them to
  ezInputManager::SetInputActionConfig(binding.m_sInputSet, binding.m_sAction, binding.m_DefaultConfig, true);

  m_bUnsavedChanges = true;
  m_bDefaultsRestored = false;
}

void ezInputRebinding::RestoreDefaults()
{
  CancelCapture();

  m_bUnsavedChanges = true;
  m_bDefaultsRestored = true;

  for (const auto& binding : m_Bindings)
  {
    ezInputManager::SetInputActionConfig(binding.m_sInputSet, binding.m_sAction, binding.m_DefaultConfig, true);
  }
}

void ezInputRebinding::SaveUserBindings()
{
  m_bUnsavedChanges = false;

  if (m_bDefaultsRestored)
  {
    // deleted rather than overwritten, so that later changes to the project's configuration reach the player
    ezFileSystem::DeleteFile(ezGameAppInputConfig::s_sUserConfigFile);
    return;
  }

  ezHybridArray<ezGameAppInputConfig, 32> configs;
  configs.Reserve(m_Bindings.GetCount());

  for (const auto& binding : m_Bindings)
  {
    const ezInputActionConfig cfg = ezInputManager::GetInputActionConfig(binding.m_sInputSet, binding.m_sAction);

    auto& config = configs.ExpandAndGetRef();
    config.m_sInputSet = binding.m_sInputSet;
    config.m_sInputAction = binding.m_sAction;
    config.m_bApplyTimeScaling = cfg.m_bApplyTimeScaling;

    for (ezUInt32 i = 0; i < MaxSlots; ++i)
    {
      config.m_sInputSlotTrigger[i] = cfg.m_sInputSlotTrigger[i];
      config.m_fInputSlotScale[i] = cfg.m_fInputSlotScale[i];
    }
  }

  ezFileWriter file;
  if (file.Open(ezGameAppInputConfig::s_sUserConfigFile).Failed())
  {
    ezLog::Warning("Failed to write the input bindings to '{}'.", ezGameAppInputConfig::s_sUserConfigFile);
    return;
  }

  ezGameAppInputConfig::WriteToDDL(file, configs);
}

void ezInputRebinding::StartCapture(ezUInt32 uiBinding, ezUInt32 uiSlot)
{
  if (uiBinding >= m_Bindings.GetCount() || uiSlot >= MaxSlots)
    return;

  m_iCaptureBinding = (ezInt32)uiBinding;
  m_iCaptureSlot = (ezInt32)uiSlot;
  m_bStartInputReleased = false;
}

void ezInputRebinding::CancelCapture()
{
  m_iCaptureBinding = -1;
  m_iCaptureSlot = -1;
  m_sReleasePending.Clear();
}

bool ezInputRebinding::UpdateCapture()
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "The input state may only be read from the main thread.");

  if (!m_sReleasePending.IsEmpty())
  {
    // Up rather than Released: in the frame where it is released, the UI would still see the release (e.g. a click)
    if (ezInputManager::GetInputSlotState(m_sReleasePending) == ezKeyState::Up)
    {
      m_sReleasePending.Clear();
    }
  }

  if (!IsCapturing())
    return false;

  // an axis can be bound to an action, but it can't be 'pressed', so it can't be picked up this way
  const ezStringView sPressed = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::Pressable, ezInputSlotFlags::ValuesAreNonContinuous);

  if (!m_bStartInputReleased)
  {
    // wait until the click or key that started the capture is released
    m_bStartInputReleased = sPressed.IsEmpty();
    return false;
  }

  if (sPressed.IsEmpty())
    return false;

  const ezUInt32 uiBinding = (ezUInt32)m_iCaptureBinding;
  const ezUInt32 uiSlot = (ezUInt32)m_iCaptureSlot;

  CancelCapture();

  m_sReleasePending = sPressed;

  if (sPressed != ezInputSlot_KeyEscape)
  {
    SetBinding(uiBinding, uiSlot, sPressed);
  }

  return true;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_InputRebinding);
