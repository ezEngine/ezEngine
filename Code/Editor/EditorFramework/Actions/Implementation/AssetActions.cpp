#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezActionDescriptorHandle ezAssetActions::s_hAssetCategory;
ezActionDescriptorHandle ezAssetActions::s_hTransformAsset;
ezActionDescriptorHandle ezAssetActions::s_hAssetHelp;
ezActionDescriptorHandle ezAssetActions::s_hTransformAllAssets;
ezActionDescriptorHandle ezAssetActions::s_hCheckFileSystem;
ezActionDescriptorHandle ezAssetActions::s_hWriteDependencyDGML;
ezActionDescriptorHandle ezAssetActions::s_hCopyAssetGuid;

void ezAssetActions::RegisterActions()
{
  s_hAssetCategory = EZ_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset = EZ_REGISTER_ACTION_1("Asset.Transform", ezActionScope::Document, "Assets", "Ctrl+E", ezAssetAction, ezAssetAction::ButtonType::TransformAsset);
  s_hAssetHelp = EZ_REGISTER_ACTION_1("Asset.Help", ezActionScope::Document, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::AssetHelp);
  s_hTransformAllAssets = EZ_REGISTER_ACTION_1("Asset.TransformAll", ezActionScope::Global, "Assets", "Ctrl+Shift+E", ezAssetAction, ezAssetAction::ButtonType::TransformAllAssets);
  s_hCheckFileSystem = EZ_REGISTER_ACTION_1("Asset.CheckFilesystem", ezActionScope::Global, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::CheckFileSystem);
  s_hWriteDependencyDGML = EZ_REGISTER_ACTION_1("Asset.WriteDependencyDGML", ezActionScope::Document, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::WriteDependencyDGML);
  s_hCopyAssetGuid = EZ_REGISTER_ACTION_1("Asset.CopyAssetGuid", ezActionScope::Document, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::CopyAssetGuid);

  {
    ezActionMap* pMap = ezActionMapManager::GetActionMap("DocumentWindowTabMenu");
    pMap->MapAction(s_hCopyAssetGuid, "", 12.0f);
  }
}

void ezAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hAssetCategory);
  ezActionManager::UnregisterAction(s_hTransformAsset);
  ezActionManager::UnregisterAction(s_hAssetHelp);
  ezActionManager::UnregisterAction(s_hTransformAllAssets);
  ezActionManager::UnregisterAction(s_hCheckFileSystem);
  ezActionManager::UnregisterAction(s_hWriteDependencyDGML);
  ezActionManager::UnregisterAction(s_hCopyAssetGuid);
}

void ezAssetActions::MapMenuActions(ezStringView sMapping)
{
  const ezStringView sTargetMenu = "G.Asset";

  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hAssetHelp, sTargetMenu, 1.0f);
  pMap->MapAction(s_hTransformAsset, sTargetMenu, 2.0f);
  pMap->MapAction(s_hCopyAssetGuid, sTargetMenu, 3.0f);
  pMap->MapAction(s_hCheckFileSystem, sTargetMenu, 4.0f);
  pMap->MapAction(s_hTransformAllAssets, sTargetMenu, 5.0f);
  pMap->MapAction(s_hWriteDependencyDGML, sTargetMenu, 6.0f);
}

void ezAssetActions::MapToolBarActions(ezStringView sMapping, bool bDocument)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
  }
  else
  {
    pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 1.0f);
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 2.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetAction::ezAssetAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  switch (m_ButtonType)
  {
    case ezAssetAction::ButtonType::TransformAsset:
      SetIconPath(":/EditorFramework/Icons/TransformAsset.svg");
      break;
    case ezAssetAction::ButtonType::TransformAllAssets:
      SetIconPath(":/EditorFramework/Icons/TransformAllAssets.svg");
      break;
    case ezAssetAction::ButtonType::CheckFileSystem:
      SetIconPath(":/EditorFramework/Icons/CheckFileSystem.svg");
      break;
    case ezAssetAction::ButtonType::AssetHelp:
      SetIconPath(":/GuiFoundation/Icons/Help.svg");
      SetEnabled(!ezTranslateHelpURL(context.m_pDocument->GetDocumentTypeName()).IsEmpty());
      break;
    case ezAssetAction::ButtonType::WriteDependencyDGML:
      break;
    case ezAssetAction::ButtonType::CopyAssetGuid:
      SetIconPath(":/GuiFoundation/Icons/Guid.svg");
      break;
  }
}

ezAssetAction::~ezAssetAction() = default;

void ezAssetAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
    case ezAssetAction::ButtonType::TransformAsset:
    {
      if (m_Context.m_pDocument->IsModified())
      {
        ezStatus res = const_cast<ezDocument*>(m_Context.m_pDocument)->SaveDocument();
        if (res.m_Result.Failed())
        {
          ezLog::Error("Failed to save document '{0}': '{1}'", m_Context.m_pDocument->GetDocumentPath(), res.m_sMessage);
          break;
        }
      }

      ezTransformStatus ret = ezAssetCurator::GetSingleton()->TransformAsset(m_Context.m_pDocument->GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);

      if (ret.Failed())
      {
        ezLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, m_Context.m_pDocument->GetDocumentPath());
      }
      else
      {
        ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
      }
    }
    break;

    case ezAssetAction::ButtonType::TransformAllAssets:
    {
      ezAssetCurator::GetSingleton()->CheckFileSystem();
      ezAssetCurator::GetSingleton()->TransformAllAssets().IgnoreResult();
    }
    break;

    case ezAssetAction::ButtonType::CheckFileSystem:
    {
      ezAssetCurator::GetSingleton()->CheckFileSystem();
      ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case ezAssetAction::ButtonType::WriteDependencyDGML:
    {
      ezStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Write to DGML", {}, "DGML (*.dgml)", nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

      if (sOutput.IsEmpty())
        return;

      ezAssetCurator::GetSingleton()->WriteDependencyDGML(m_Context.m_pDocument->GetGuid(), sOutput);
    }
    break;

    case ezAssetAction::ButtonType::AssetHelp:
    {
      ezStringView sType = GetContext().m_pDocument->GetDocumentTypeName();
      ezString sURL = ezTranslateHelpURL(sType);

      if (!sURL.IsEmpty())
      {
        QDesktopServices::openUrl(QUrl(ezMakeQString(sURL)));
      }
    }
    break;

    case ezAssetAction::ButtonType::CopyAssetGuid:
    {
      ezStringBuilder sGuid;
      ezConversionUtils::ToString(m_Context.m_pDocument->GetGuid(), sGuid);

      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(sGuid.GetData());
      clipboard->setMimeData(mimeData);

      ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Copied asset GUID: {}", sGuid), ezTime::MakeFromSeconds(5));
    }
    break;
  }
}
