#include <Core/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Core/Input/InputManager.h>

ezMap<ezString, ezInputManager::ezActionMap, ezCompareHelper<ezString>, ezStaticAllocatorWrapper > ezInputManager::s_ActionMapping;
ezMap<ezString, ezString, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> ezInputManager::s_ActionDisplayNames; 

ezInputManager::ezInputActionConfig::ezInputActionConfig()
{
  m_fFilterXMinValue = 0.0f;
  m_fFilterXMaxValue = 1.0f;
  m_fFilterYMinValue = 0.0f;
  m_fFilterYMaxValue = 1.0f;

  m_fFilteredPriority = -10000.0f;

  m_OnLeaveArea = LoseFocus;
  m_OnEnterArea = ActivateImmediately;
}

ezInputManager::ezActionData::ezActionData()
{
  m_fValue = 0.0f;
  m_State = ezKeyState::Up;
}

void ezInputManager::ClearInputMapping(const char* szInputSet, const char* szInputSlot)
{
  ezActionMap& Actions = s_ActionMapping[szInputSet];

  // iterate over all existing actions
  for (ezActionMap::Iterator it = Actions.GetIterator(); it.IsValid(); ++it)
  {
    // iterate over all input slots in the existing action
    for (ezUInt32 i1 = 0; i1 < ezInputActionConfig::MaxInputSlotAlternatives; ++i1)
    {
      // if that action is triggered by the given input slot, remove that trigger from the action

      if (it.Value().m_Config.m_sInputSlotTrigger[i1] == szInputSlot)
        it.Value().m_Config.m_sInputSlotTrigger[i1].Clear();
    }
  }
}

void ezInputManager::SetInputActionConfig(const char* szInputSet, const char* szAction, const ezInputActionConfig& Config, bool bClearPreviousInputMappings)
{
  EZ_ASSERT(!ezStringUtils::IsNullOrEmpty(szInputSet), "The InputSet name must not be empty.");
  EZ_ASSERT(!ezStringUtils::IsNullOrEmpty(szAction), "No input action to map to was given.");

  if (bClearPreviousInputMappings)
  {
    for (ezUInt32 i1 = 0; i1 < ezInputActionConfig::MaxInputSlotAlternatives; ++i1)
      ClearInputMapping(szInputSet, Config.m_sInputSlotTrigger[i1].GetData());
  }

  // store the new action mapping
  s_ActionMapping[szInputSet][szAction].m_Config = Config;
}

ezInputManager::ezInputActionConfig ezInputManager::GetInputActionConfig(const char* szInputSet, const char* szAction)
{
  const ezInputSetMap::ConstIterator ItSet = s_ActionMapping.Find(szInputSet);

  if (!ItSet.IsValid())
    return ezInputManager::ezInputActionConfig();

  const ezActionMap::ConstIterator ItAction = ItSet.Value().Find(szAction);

  if (!ItAction.IsValid())
    return ezInputManager::ezInputActionConfig();

  return ItAction.Value().m_Config;
}

void ezInputManager::RemoveInputAction(const char* szInputSet, const char* szAction)
{
  s_ActionMapping[szInputSet].Erase(szAction);
}

ezKeyState::Enum ezInputManager::GetInputActionState(const char* szInputSet, const char* szAction, float* pValue)
{
  if (pValue)
    *pValue = 0.0f;

  const ezInputSetMap::ConstIterator ItSet = s_ActionMapping.Find(szInputSet);

  if (!ItSet.IsValid())
    return ezKeyState::Up;

  const ezActionMap::ConstIterator ItAction = ItSet.Value().Find(szAction);

  if (!ItAction.IsValid())
    return ezKeyState::Up;

  if (pValue)
    *pValue = ItAction.Value().m_fValue;

  return ItAction.Value().m_State;
}

ezInputManager::ezActionMap::Iterator ezInputManager::GetBestAction(ezActionMap& Actions, const ezString& sSlot, const ezActionMap::Iterator& itFirst)
{
  // this function determines which input action should be triggered by the given input slot
  // it will prefer actions with higher priority
  // it will check that all conditions of the action are met (ie. filters like that a mouse cursor is inside a rectangle)
  // if some action had focus before and shall keep it until some key is released, that action will always be preferred

  ezActionMap::Iterator ItAction = itFirst;

  ezActionMap::Iterator itBestAction;
  float fBestPriority = -1000000;

  if (ItAction.IsValid())
  {
    // otherwise take the priority of the last returned value as the basis to compare all other actions against
    fBestPriority = ItAction.Value().m_Config.m_fFilteredPriority;

    // and make sure to skip the last returned action, of course
    ++ItAction;
  }
  else
  {
    // if an invalid iterator is passed in, this is the first call to this function, start searching a the very beginning
    ItAction = Actions.GetIterator();
  }

  // check all actions from the given array
  for ( ; ItAction.IsValid(); ++ItAction)
  {
    ezActionData& ThisAction = ItAction.Value();

    // if we the given slot triggers this action (over any of its alternative slots), continue
    for (ezUInt32 AltSlot = 0; AltSlot < ezInputActionConfig::MaxInputSlotAlternatives; ++AltSlot)
    {
      if (ThisAction.m_Config.m_sInputSlotTrigger[AltSlot] == sSlot)
        goto hell;
    }

    // if the action is not triggered by this slot, skip it
    continue;

hell:

    // if the action had input in the last update AND wants to keep the focus, it will ALWAYS get the input, until the input slot gets inactive (key up)
    // independent from priority, overlap of areas etc.
    if (ThisAction.m_State != ezKeyState::Up && ThisAction.m_Config.m_OnLeaveArea == ezInputActionConfig::KeepFocus)
    {
      // just return this result immediately
      return ItAction;
    }

    // if this action has lower priority than what we already found, ignore it
    if (ThisAction.m_Config.m_fFilteredPriority < fBestPriority)
      continue;

    // if it has the same priority but we already found one, also ignore it
    // if it has the same priority but we did not yet find a 'best action' take this one
    if (ThisAction.m_Config.m_fFilteredPriority == fBestPriority && itBestAction.IsValid())
      continue;

    // this is the "mouse cursor filter" for the x-axis
    // if any filter is set, check that it is in range
    if (!ThisAction.m_Config.m_sFilterByInputSlotX.IsEmpty())
    {
      const float fVal = s_InputSlots[ThisAction.m_Config.m_sFilterByInputSlotX].m_fValue;
      if (fVal < ThisAction.m_Config.m_fFilterXMinValue || fVal > ThisAction.m_Config.m_fFilterXMaxValue)
        continue;
    }

    // this is the "mouse cursor filter" for the y-axis
    // if any filter is set, check that it is in range
    if (!ThisAction.m_Config.m_sFilterByInputSlotY.IsEmpty())
    {
      const float fVal = s_InputSlots[ThisAction.m_Config.m_sFilterByInputSlotY].m_fValue;
      if (fVal < ThisAction.m_Config.m_fFilterYMinValue || fVal > ThisAction.m_Config.m_fFilterYMaxValue)
        continue;
    }

    // we found something!
    fBestPriority = ThisAction.m_Config.m_fFilteredPriority;
    itBestAction = ItAction;
  }

  return itBestAction;
}

void ezInputManager::UpdateInputActions()
{
  // update each input set
  // all input sets are disjunct from each other, so one key press can have different effects in each input set
  for (ezInputSetMap::Iterator ItSets = s_ActionMapping.GetIterator(); ItSets.IsValid(); ++ItSets)
  {
    UpdateInputActions(ItSets.Value());
  }
}

void ezInputManager::UpdateInputActions(ezActionMap& Actions)
{
  // reset all action values to zero
  for (ezActionMap::Iterator ItActions = Actions.GetIterator(); ItActions.IsValid(); ++ItActions)
    ItActions.Value().m_fValue = 0.0f;

  // iterate over all input slots and check how their values affect the actions from the current input set
  for (ezInputSlotsMap::Iterator ItSlots = s_InputSlots.GetIterator(); ItSlots.IsValid(); ++ItSlots)
  {
    // if this input slot is not active, ignore it; we will reset all actions later
    if (ItSlots.Value().m_fValue == 0.0f)
      continue;

    // If this key got clicked in this frame, it has not been dragged into the active area of the action
    // e.g. the mouse has been clicked while it was inside this area, instead of outside and then moved here
    const bool bFreshClick = ItSlots.Value().m_State == ezKeyState::Pressed;

    // find the action that should be affected by this input slot
    ezActionMap::Iterator itBestAction;
    
    // we activate all actions with the same priority simultaneously
    while (true)
    {
      // get the (next) best action
      itBestAction = GetBestAction(Actions, ItSlots.Key(), itBestAction);

      // if we found anything, update its input
      if (!itBestAction.IsValid())
        break;

      const float fNewValue = ezMath::Max(itBestAction.Value().m_fValue, ItSlots.Value().m_fValue);

      if (itBestAction.Value().m_Config.m_OnEnterArea == ezInputActionConfig::RequireKeyUp)
      {
        // if this action requires that it is only activated by a key press while the mouse is inside it
        // we check whether this is either a fresh click (inside the area) or the action is already active
        // if it is already active, the mouse is most likely held clicked atm

        if (bFreshClick || (itBestAction.Value().m_fValue > 0.0f))
          itBestAction.Value().m_fValue = fNewValue;
      }
      else
        itBestAction.Value().m_fValue = fNewValue;
    }
  }

  // now update all action states, if any one has not gotten any input from any input slot recently, it will be reset to 'Released' or 'Up'
  for (ezActionMap::Iterator ItActions = Actions.GetIterator(); ItActions.IsValid(); ++ItActions)
  {
    const bool bHasInput = ItActions.Value().m_fValue > 0.0f;
    ItActions.Value().m_State = ezKeyState::GetNewKeyState(ItActions.Value().m_State, bHasInput);
  }
}

void ezInputManager::SetActionDisplayName(const char* szAction, const char* szDisplayName)
{
  s_ActionDisplayNames[szAction] = szDisplayName;
}

const char* ezInputManager::GetActionDisplayName(const char* szAction)
{
  ezMap<ezString, ezString, ezCompareHelper<ezString>, ezStaticAllocatorWrapper>::Iterator it = s_ActionDisplayNames.Find(szAction);

  if (it.IsValid())
    return it.Value().GetData();

  return szAction;
}

void ezInputManager::GetAllInputSets(ezDynamicArray<ezString>& out_InputSetNames)
{
  for (ezInputSetMap::Iterator it = s_ActionMapping.GetIterator(); it.IsValid(); ++it)
    out_InputSetNames.PushBack(it.Key());
}

void ezInputManager::GetAllInputActions(const char* szInputSetName, ezDynamicArray<ezString>& out_InputActions)
{
  for (ezActionMap::Iterator it = s_ActionMapping[szInputSetName].GetIterator(); it.IsValid(); ++it)
    out_InputActions.PushBack(it.Key());
}






