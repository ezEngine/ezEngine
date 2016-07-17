#include <PCH.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>

ezActionDescriptorHandle ezViewActions::s_hRenderMode;
ezActionDescriptorHandle ezViewActions::s_hPerspective;
ezActionDescriptorHandle ezViewActions::s_hRenderPipeline;


void ezViewActions::RegisterActions()
{
  s_hRenderMode = EZ_REGISTER_LRU_MENU("View.RenderMode", ezRenderModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hPerspective = EZ_REGISTER_LRU_MENU("View.RenderPerspective", ezPerspectiveAction, ":/EditorFramework/Icons/Perspective.png");
  s_hRenderPipeline = EZ_REGISTER_LRU_MENU("View.RenderPipeline", ezRenderPipelineMenuAction, ":/AssetIcons/RenderPipeline.png");
}

void ezViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hRenderMode);
  ezActionManager::UnregisterAction(s_hPerspective);
  ezActionManager::UnregisterAction(s_hRenderPipeline);
}

void ezViewActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hPerspective, szPath, 1.0f);
  pMap->MapAction(s_hRenderMode, szPath, 2.0f);
  pMap->MapAction(s_hRenderPipeline, szPath, 3.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRenderModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderModeAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderModeAction::ezRenderModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezViewRenderMode>());
}

ezInt64 ezRenderModeAction::GetValue() const
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_RenderMode;
}

void ezRenderModeAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_RenderMode = (ezViewRenderMode::Enum)value.ConvertTo<ezInt64>();
  TriggerUpdate();
}

////////////////////////////////////////////////////////////////////////
// ezPerspectiveAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPerspectiveAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPerspectiveAction::ezPerspectiveAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezSceneViewPerspective>());
}

ezInt64 ezPerspectiveAction::GetValue() const
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_Perspective;
}

void ezPerspectiveAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  auto newValue = (ezSceneViewPerspective::Enum)value.ConvertTo<ezInt64>();

  if (pView->m_pViewConfig->m_Perspective != newValue)
  {
    pView->m_pViewConfig->m_Perspective = newValue;
    pView->m_pViewConfig->ApplyPerspectiveSetting();
    TriggerUpdate();
  }
}


////////////////////////////////////////////////////////////////////////
// ezRenderPipelineMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderPipelineMenuAction::ezRenderPipelineMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezLRUMenuAction(context, szName, szIconPath)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtEngineViewWidget'!");
}

void ezRenderPipelineMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
 
  out_Entries.Clear();
  {
    ezLRUMenuAction::Item browse;
    browse.m_sDisplay = "Browse...";
    browse.m_CheckState = ezLRUMenuAction::Item::CheckMark::NotCheckable;
    browse.m_UserValue = (ezInt64)Browse;
    out_Entries.PushBack(browse);
  }
  {
    bool bIsDefault = pView->m_pViewConfig->m_sRenderPipelineResource.IsEmpty();
    ezLRUMenuAction::Item default;
    default.m_sDisplay = "Default";
    default.m_CheckState = bIsDefault ? ezLRUMenuAction::Item::CheckMark::Checked : ezLRUMenuAction::Item::CheckMark::Unchecked;
    default.m_UserValue = (ezInt64)Default;
    out_Entries.PushBack(default);
  }

  {
    ezLRUMenuAction::Item browse;
    browse.m_ItemFlags = ezLRUMenuAction::Item::ItemFlags::Separator;
    out_Entries.PushBack(browse);
    
  }
  ezHybridArray<ezString, 10> list;
  GetRecentRenderPipelines(list);
  for (ezString& sEntry : list)
  {
    ezLRUMenuAction::Item entryItem;
    entryItem.m_sDisplay = sEntry;
    if (ezConversionUtils::IsStringUuid(sEntry))
    {
      ezUuid assetGuid = ezConversionUtils::ConvertStringToUuid(sEntry);
      const auto* pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo(assetGuid);

      if (pAsset)
      {
        entryItem.m_sDisplay = pAsset->m_sDataDirRelativePath;
      }
    }
    entryItem.m_CheckState = (pView->m_pViewConfig->m_sRenderPipelineResource == sEntry) ? ezLRUMenuAction::Item::CheckMark::Checked : ezLRUMenuAction::Item::CheckMark::Unchecked;
    entryItem.m_UserValue = sEntry;
    entryItem.m_Icon = QIcon(QLatin1String(":/AssetIcons/RenderPipeline.png"));
    out_Entries.PushBack(entryItem);
  }
}

void ezRenderPipelineMenuAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);

  if (value.IsA<ezString>())
  {
    AddToRecentRenderPipelines(value.Get<ezString>());
    pView->m_pViewConfig->m_sRenderPipelineResource = value.Get<ezString>();
    pView->m_pViewConfig->ApplyPerspectiveSetting();
    TriggerUpdate();
  }
  else if (value.IsA<ezInt64>())
  {
    auto newValue = (ezRenderPipelineMenuAction::Actions)value.ConvertTo<ezInt64>();

    if (newValue == Browse)
    {
      ezString sFile = pView->m_pViewConfig->m_sRenderPipelineResource;

      ezAssetBrowserDlg dlg(pView, sFile, "RenderPipeline");
      if (dlg.exec() == 0)
        return;

      sFile = dlg.GetSelectedAssetGuid();

      if (sFile.IsEmpty())
      {
        sFile = dlg.GetSelectedAssetPathRelative();

        if (sFile.IsEmpty())
        {
          sFile = dlg.GetSelectedAssetPathAbsolute();

          ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
        }
      }

      if (sFile.IsEmpty())
        return;

      AddToRecentRenderPipelines(sFile);
      pView->m_pViewConfig->m_sRenderPipelineResource = sFile;
      pView->m_pViewConfig->ApplyPerspectiveSetting();
      TriggerUpdate();
    }
    else if (newValue == Default)
    {
      pView->m_pViewConfig->m_sRenderPipelineResource.Clear();
      pView->m_pViewConfig->ApplyPerspectiveSetting();
      TriggerUpdate();
    }
  }

}

void ezRenderPipelineMenuAction::GetRecentRenderPipelines(ezHybridArray<ezString, 10>& list)
{
  ezProjectPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
  ezStringBuilder sList = pPreferences->m_sRenderPipelines;

  list.Clear();
  sList.Split(false, list, ";");
}

void ezRenderPipelineMenuAction::AddToRecentRenderPipelines(const ezString& entry)
{
  ezHybridArray<ezString, 10> list;
  GetRecentRenderPipelines(list);

  ezUInt32 uiIndex = list.IndexOf(entry);
  if (uiIndex != ezInvalidIndex)
  {
    list.RemoveAt(uiIndex);
  }

  list.Insert(entry, 0);

  ezStringBuilder sList;
  for (ezString& sEntry : list)
  {
    sList.Append(";", sEntry);
  }

  ezProjectPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
  pPreferences->m_sRenderPipelines = sList;
}
