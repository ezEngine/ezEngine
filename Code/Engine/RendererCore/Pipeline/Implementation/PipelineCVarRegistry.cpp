#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Pipeline/PipelineCVarRegistry.h>
#include <Core/Utils/Blackboard.h>


// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, PipelineCVarRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "CVars"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezPipelineCVarRegistry::UpdateCVars();
    ezCVar::s_AllCVarEvents.AddEventHandler(ezPipelineCVarRegistry::CVarListChangedEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezCVar::s_AllCVarEvents.RemoveEventHandler(ezPipelineCVarRegistry::CVarListChangedEventHandler);
    ezPipelineCVarRegistry::ClearCVars();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezCVar::CVarEvents ezPipelineCVarRegistry::s_CVarChangedEvent;
ezDynamicArray<ezCVar*> ezPipelineCVarRegistry::s_CVars;
ezDynamicArray<ezEventSubscriptionID> ezPipelineCVarRegistry::s_CVarEventSubscriptions;
ezHashTable<const ezCVar*, ezHashedString> ezPipelineCVarRegistry::s_CVarStrings;

ezArrayPtr<ezCVar* const> ezPipelineCVarRegistry::GetCVars()
{
  return ezArrayPtr<ezCVar* const>(s_CVars.GetData(), s_CVars.GetCount());
}

void ezPipelineCVarRegistry::ApplyCVarsToBlackboard(ezBlackboard* pBlackboard)
{
  for (ezUInt32 i=0; i < s_CVars.GetCount(); ++i)
  {
    ApplyCVarToBlackboard(pBlackboard, s_CVars[i]);
  }
}

void ezPipelineCVarRegistry::ApplyCVarToBlackboard(ezBlackboard* pBlackboard, const ezCVar* pCVar)
{
  auto it = s_CVarStrings.Find(pCVar);
  if (!it.IsValid())
  {
    EZ_REPORT_FAILURE("The CVar '{}' is not a pipeline cvar", pCVar->GetName());
    return;
  }

  switch (pCVar->GetType())
  {
    case ezCVarType::String:
      pBlackboard->SetEntryValue(it.Value(), static_cast<const ezCVarString*>(pCVar)->GetValue());
      break;
    case ezCVarType::Int:
      pBlackboard->SetEntryValue(it.Value(), static_cast<const ezCVarInt*>(pCVar)->GetValue());
      break;
    case ezCVarType::Float:
      pBlackboard->SetEntryValue(it.Value(), static_cast<const ezCVarFloat*>(pCVar)->GetValue());
      break;
    case ezCVarType::Bool:
      pBlackboard->SetEntryValue(it.Value(), static_cast<const ezCVarBool*>(pCVar)->GetValue());
      break;
    default:
      EZ_REPORT_FAILURE("Unknown cvar type");
      break;
  }
}

void ezPipelineCVarRegistry::CVarListChangedEventHandler(const ezCVarEvent& e)
{
  if (e.m_EventType == ezCVarEvent::ListOfVarsChanged)
  {
    UpdateCVars();
  }
}

void ezPipelineCVarRegistry::CVarChangedEventHandler(const ezCVarEvent& e)
{
  s_CVarChangedEvent.Broadcast(e);
}

void ezPipelineCVarRegistry::UpdateCVars()
{
  ezHashSet<ezCVar*, ezHashHelper<ezCVar*>, ezTempAllocatorWrapper> newCVars;

  for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (pCVar->GetName().StartsWith("Rendering.Pipeline."))
    {
      newCVars.Insert(pCVar);
    }
  }

  for (ezUInt32 i = s_CVars.GetCount(); i > 0; --i)
  {
    const ezUInt32 uiIndex = i - 1;

    auto it = newCVars.Find(s_CVars[uiIndex]);
    if (it.IsValid())
    {
      s_CVars[uiIndex]->m_CVarEvents.RemoveEventHandler(s_CVarEventSubscriptions[uiIndex]);
      s_CVars.RemoveAtAndCopy(uiIndex);
      s_CVarEventSubscriptions.RemoveAtAndCopy(uiIndex);
      s_CVarStrings.Remove(s_CVars[uiIndex]);
      newCVars.Remove(it);
    }
  }

  for (ezCVar* pCVar : newCVars)
  {
    s_CVars.PushBack(pCVar);
    s_CVarEventSubscriptions.PushBack(pCVar->m_CVarEvents.AddEventHandler(CVarChangedEventHandler));
    ezHashedString sName;
    sName.Assign(pCVar->GetName());
    s_CVarStrings.Insert(pCVar, sName);

    ezCVarEvent e(pCVar);
    e.m_EventType = ezCVarEvent::ValueChanged;
    s_CVarChangedEvent.Broadcast(e);
  }
}

void ezPipelineCVarRegistry::ClearCVars()
{
  for (ezUInt32 i = 0; i < s_CVars.GetCount(); ++i)
  {
    s_CVars[i]->m_CVarEvents.RemoveEventHandler(s_CVarEventSubscriptions[i]);
  }

  s_CVars.Clear();
  s_CVars.Compact();
  s_CVarEventSubscriptions.Clear();
  s_CVarEventSubscriptions.Compact();
  s_CVarStrings.Clear();
  s_CVarStrings.Compact();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_PipelineCVarRegistry);
