#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiScript.h>
#include <AiPlugin/UtilityAI/Impl/GameAiActions.h>
#include <AiPlugin/UtilityAI/Impl/GameAiScriptActions.h>
#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAiScriptActions, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueNavigateTo, In, "Context", In, "TargetPosition", In, "Speed", In, "AllowPartialPath")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(5.0f)),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(true))),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueNavigateToObject, In, "Context", In, "TargetObject", In, "Speed", In, "AllowPartialPath")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezDefaultValueAttribute(5.0f)),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(true))),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueStopWalking, In, "Context"),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueTurnTowards, In, "Context", In, "TargetPosition"),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueWait, In, "Context", In, "Duration")->AddAttributes(
      new ezFunctionArgumentAttributes(1, new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1)))),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueBlackboardSet, In, "Context", In, "EntryName", In, "Value")->AddAttributes(
      new ezFunctionArgumentAttributes(1, new ezDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueBlackboardWait, In, "Context", In, "EntryName", In, "Value", In, "Equals")->AddAttributes(
      new ezFunctionArgumentAttributes(1, new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(true))),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueBlackboardSetAndWait, In, "Context", In, "EntryName", In, "SetValue", In, "WaitValue", In, "Timeout")->AddAttributes(
      new ezFunctionArgumentAttributes(1, new ezDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    EZ_SCRIPT_FUNCTION_PROPERTY(QueueLog, In, "Context", In, "Text"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("AiActions"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

static ezAiActionQueue* GetActionQueue(ezAiScriptContext* pContext, ezStringView sFunction)
{
  if (pContext == nullptr || pContext->m_pActionQueue == nullptr)
  {
    ezLog::Warning("AiActions::{}: actions can only be queued in 'OnActivateBehavior' and 'OnReactivateBehavior' of a script behavior.", sFunction);
    return nullptr;
  }

  return pContext->m_pActionQueue;
}

void ezAiScriptActions::QueueNavigateTo(ezAiScriptContext* pContext, const ezVec3& vTargetPosition, float fSpeed, bool bAllowPartialPath)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueNavigateTo"))
  {
    auto pAction = ezAiActionNavigateTo::Create();
    pAction->m_vTargetPosition = vTargetPosition;
    pAction->m_fSpeed = fSpeed;
    pAction->m_bAllowPartialPath = bAllowPartialPath;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueNavigateToObject(ezAiScriptContext* pContext, ezGameObjectHandle hTargetObject, float fSpeed, bool bAllowPartialPath)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueNavigateToObject"))
  {
    auto pAction = ezAiActionNavigateTo::Create();
    pAction->m_hTargetObject = hTargetObject;
    pAction->m_fSpeed = fSpeed;
    pAction->m_bAllowPartialPath = bAllowPartialPath;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueStopWalking(ezAiScriptContext* pContext)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueStopWalking"))
  {
    pQueue->QueueAction(ezAiActionStopWalking::Create());
  }
}

void ezAiScriptActions::QueueTurnTowards(ezAiScriptContext* pContext, const ezVec3& vTargetPosition)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueTurnTowards"))
  {
    auto pAction = ezAiActionTurnTowards::Create();
    pAction->m_vTargetPosition = vTargetPosition;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueWait(ezAiScriptContext* pContext, ezTime duration)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueWait"))
  {
    auto pAction = ezAiActionWait::Create();
    pAction->m_Duration = duration;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueBlackboardSet(ezAiScriptContext* pContext, ezStringView sEntryName, const ezVariant& value)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueBlackboardSet"))
  {
    auto pAction = ezAiActionBlackboardSetEntry::Create();
    pAction->m_sEntryName.Assign(sEntryName);
    pAction->m_Value = value;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueBlackboardWait(ezAiScriptContext* pContext, ezStringView sEntryName, const ezVariant& value, bool bEquals)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueBlackboardWait"))
  {
    auto pAction = ezAiActionBlackboardWait::Create();
    pAction->m_sEntryName = ezTempHashedString(sEntryName);
    pAction->m_Value = value;
    pAction->m_bEquals = bEquals;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueBlackboardSetAndWait(ezAiScriptContext* pContext, ezStringView sEntryName, const ezVariant& setValue, const ezVariant& waitValue, ezTime timeout)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueBlackboardSetAndWait"))
  {
    auto pAction = ezAiActionBlackboardSetAndWait::Create();
    pAction->m_sEntryName.Assign(sEntryName);
    pAction->m_SetValue = setValue;
    pAction->m_WaitValue = waitValue;
    pAction->m_Timeout = timeout;
    pQueue->QueueAction(pAction);
  }
}

void ezAiScriptActions::QueueLog(ezAiScriptContext* pContext, ezStringView sText)
{
  if (ezAiActionQueue* pQueue = GetActionQueue(pContext, "QueueLog"))
  {
    auto pAction = ezAiActionQuip::Create();
    pAction->m_sLogMsg = sText;
    pQueue->QueueAction(pAction);
  }
}
