#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionManager)

BEGIN_SUBSYSTEM_DEPENDENCIES
"ToolsFoundation"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezActionManager::Startup();
}

ON_CORE_SHUTDOWN
{
  ezActionManager::Shutdown();
}

EZ_END_SUBSYSTEM_DECLARATION

ezEvent<const ezActionManager::Event&> ezActionManager::s_Events;
ezIdTable<ezActionId, ezActionDescriptor*> ezActionManager::s_ActionTable;
ezMap<ezString, ezActionManager::CategoryData> ezActionManager::s_CategoryPathToActions;
ezMap<ezString, ezString> ezActionManager::s_ShortcutOverride;

////////////////////////////////////////////////////////////////////////
// ezActionManager public functions
////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezActionManager::RegisterAction(const ezActionDescriptor& desc)
{
  ezActionDescriptorHandle hType = GetActionHandle(desc.m_sCategoryPath, desc.m_sActionName);
  EZ_ASSERT_DEV(hType.IsInvalidated(), "The action '{0}' in category '{1}' was already registered!", desc.m_sActionName.GetData(), desc.m_sCategoryPath.GetData());

  ezActionDescriptor* pDesc = CreateActionDesc(desc);

  // apply shortcut override
  {
    auto ovride = s_ShortcutOverride.Find(desc.m_sActionName);
    if (ovride.IsValid())
      pDesc->m_sShortcut = ovride.Value();
  }

  hType = ezActionDescriptorHandle(s_ActionTable.Insert(pDesc));
  pDesc->m_Handle = hType;

  auto it = s_CategoryPathToActions.FindOrAdd(pDesc->m_sCategoryPath);
  it.Value().m_Actions.Insert(hType);
  it.Value().m_ActionNameToHandle[pDesc->m_sActionName.GetData()] = hType;

  {
    Event msg;
    msg.m_Type = Event::Type::ActionAdded;
    msg.m_pDesc = pDesc;
    msg.m_Handle = hType;
    s_Events.Broadcast(msg);
  }
  return hType;
}

bool ezActionManager::UnregisterAction(ezActionDescriptorHandle& hAction)
{
  ezActionDescriptor* pDesc = nullptr;
  if (!s_ActionTable.TryGetValue(hAction, pDesc))
  {
    hAction.Invalidate();
    return false;
  }

  auto it = s_CategoryPathToActions.Find(pDesc->m_sCategoryPath);
  EZ_ASSERT_DEV(it.IsValid(), "Action is present but not mapped in its category path!");
  EZ_VERIFY(it.Value().m_Actions.Remove(hAction), "Action is present but not in its category data!");
  EZ_VERIFY(it.Value().m_ActionNameToHandle.Remove(pDesc->m_sActionName), "Action is present but its name is not in the map!");
  if (it.Value().m_Actions.IsEmpty())
  {
    s_CategoryPathToActions.Remove(it);
  }

  s_ActionTable.Remove(hAction);
  DeleteActionDesc(pDesc);
  hAction.Invalidate();
  return true;
}

const ezActionDescriptor* ezActionManager::GetActionDescriptor(ezActionDescriptorHandle hAction)
{
  ezActionDescriptor* pDesc = nullptr;
  if (s_ActionTable.TryGetValue(hAction, pDesc))
    return pDesc;

  return nullptr;
}

const ezIdTable<ezActionId, ezActionDescriptor*>::ConstIterator ezActionManager::GetActionIterator()
{
  return s_ActionTable.GetIterator();
}

ezActionDescriptorHandle ezActionManager::GetActionHandle(const char* szCategoryPath, const char* szActionName)
{
  ezActionDescriptorHandle hAction;
  auto it = s_CategoryPathToActions.Find(szCategoryPath);
  if (!it.IsValid())
    return hAction;

  it.Value().m_ActionNameToHandle.TryGetValue(szActionName, hAction);

  return hAction;
}


void ezActionManager::SaveShortcutAssignment()
{
  ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  EZ_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  ezDeferredFileWriter file;
  file.SetOutput(sFile);

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);

  ezStringBuilder sKey;

  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != ezActionType::Action)
      continue;

    if (pAction->m_sShortcut == pAction->m_sDefaultShortcut)
      sKey.Set("default: ", pAction->m_sShortcut);
    else
      sKey = pAction->m_sShortcut;

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::String, pAction->m_sActionName);
    writer.WriteString(sKey);
    writer.EndPrimitiveList();
  }

  if (file.Close().Failed())
  {
    ezLog::Error("Failed to write shortcuts config file '{0}'", sFile.GetData());
  }
}

void ezActionManager::LoadShortcutAssignment()
{
  ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  EZ_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  ezFileReader file;
  if (file.Open(sFile).Failed())
  {
    ezLog::Dev("No shortcuts file '{0}' was found", sFile.GetData());
    return;
  }

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, ezLog::GetThreadLocalLogSystem()).Failed())
    return;

  const auto obj = reader.GetRootElement();

  ezStringBuilder sKey, sValue;

  for (auto pElement = obj->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (!pElement->HasName() || !pElement->HasPrimitives(ezOpenDdlPrimitiveType::String))
      continue;

    sKey = pElement->GetName();
    sValue = pElement->GetPrimitivesString()[0];

    if (sValue.FindSubString_NoCase("default") != nullptr)
      continue;

    s_ShortcutOverride[sKey] = sValue;
  }

  // apply overrides
  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != ezActionType::Action)
      continue;

    auto ovride = s_ShortcutOverride.Find(pAction->m_sActionName);
    if (ovride.IsValid())
      pAction->m_sShortcut = ovride.Value();
  }
}

////////////////////////////////////////////////////////////////////////
// ezActionManager private functions
////////////////////////////////////////////////////////////////////////

void ezActionManager::Startup()
{
  ezDocumentActions::RegisterActions();
  ezStandardMenus::RegisterActions();
  ezCommandHistoryActions::RegisterActions();
  ezEditActions::RegisterActions();
}

void ezActionManager::Shutdown()
{
  ezDocumentActions::UnregisterActions();
  ezStandardMenus::UnregisterActions();
  ezCommandHistoryActions::UnregisterActions();
  ezEditActions::UnregisterActions();
}

ezActionDescriptor* ezActionManager::CreateActionDesc(const ezActionDescriptor& desc)
{
  ezActionDescriptor* pDesc = EZ_DEFAULT_NEW(ezActionDescriptor);
  *pDesc = desc;
  return pDesc;
}

void ezActionManager::DeleteActionDesc(ezActionDescriptor* pDesc)
{
  EZ_DEFAULT_DELETE(pDesc);
}
