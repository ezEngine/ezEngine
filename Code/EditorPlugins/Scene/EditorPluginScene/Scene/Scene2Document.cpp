#include <EditorPluginScenePCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneLayerBase, 1, ezRTTINoAllocator)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneLayer, 1, ezRTTIDefaultAllocator<ezSceneLayer>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Layer", m_Layer)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentSettings, 2, ezRTTIDefaultAllocator<ezSceneDocumentSettings>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Layers", m_Layers)->AddFlags(ezPropertyFlags::PointerOwner)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScene2Document, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


ezSceneLayerBase::ezSceneLayerBase()
{
}

ezSceneLayerBase::~ezSceneLayerBase()
{
}

//////////////////////////////////////////////////////////////////////////

ezSceneLayer::ezSceneLayer()
{
}

ezSceneLayer::~ezSceneLayer()
{
}

//////////////////////////////////////////////////////////////////////////

ezSceneDocumentSettings::ezSceneDocumentSettings()
{
}

ezSceneDocumentSettings::~ezSceneDocumentSettings()
{
  for (ezSceneLayerBase* pLayer : m_Layers)
  {
    EZ_DEFAULT_DELETE(pLayer);
  }
}

ezScene2Document::ezScene2Document(const char* szDocumentPath)
  : ezSceneDocument(szDocumentPath, ezSceneDocument::DocumentType::Scene)
{
  // Separate selection for the layer panel.
  m_LayerSelection.SetOwner(m_pObjectManager.Borrow());
}

ezScene2Document::~ezScene2Document()
{
  SetActiveLayer(GetGuid()).LogFailure();

  m_SelectionManager->SetOwner(nullptr);
  // Move the preserved real scene document back.
  m_pObjectManager = std::move(m_pSceneObjectManager);
  m_CommandHistory = std::move(m_pSceneCommandHistory);
  m_SelectionManager = std::move(m_sceneSelectionManager);
  m_ObjectAccessor = std::move(m_pSceneObjectAccessor);

  m_documentManagerEventSubscriber.Unsubscribe();
  m_layerSelectionEventSubscriber.Unsubscribe();
  m_structureEventSubscriber.Unsubscribe();
  m_commandHistoryEventSubscriber.Unsubscribe();
}

void ezScene2Document::InitializeAfterLoading(bool bFirstTimeCreation)
{
  EnsureSettingsObjectExist();

  //#TODO: Test code to add some layers
  m_ActiveLayerGuid = GetGuid();
  ezObjectDirectAccessor accessor(GetObjectManager());
  ezObjectAccessorBase* pAccessor = &accessor;
  auto pRoot = GetObjectManager()->GetObject(GetSettingsObject()->GetGuid());
  if (pRoot->GetChildren().IsEmpty())
  {
    ezUuid objectGuid;
    pAccessor->AddObject(pRoot, "Layers", 0, ezGetStaticRTTI<ezSceneLayer>(), objectGuid);
    const ezDocumentObject* pObject = pAccessor->GetObject(objectGuid);
    pAccessor->SetValue(pObject, "Layer", GetGuid());
  }

  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void ezScene2Document::InitializeAfterLoadingAndSaving()
{
  m_LayerSelection.m_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::LayerSelectionEventHandler, this), m_layerSelectionEventSubscriber);
  m_pObjectManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezScene2Document::StructureEventHandler, this), m_structureEventSubscriber);
  m_CommandHistory->m_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::CommandHistoryEventHandler, this), m_commandHistoryEventSubscriber);
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::DocumentManagerEventHandler, this), m_documentManagerEventSubscriber);

  SUPER::InitializeAfterLoadingAndSaving();
  // These preserve the real scene document.
  m_pSceneObjectManager = std::move(m_pObjectManager);
  m_pSceneCommandHistory = std::move(m_CommandHistory);
  m_sceneSelectionManager = std::move(m_SelectionManager);
  m_pSceneObjectAccessor = std::move(m_ObjectAccessor);

  // Replace real scene elements with copies.
  m_pObjectManager = EZ_DEFAULT_NEW(ezSceneObjectManager);
  m_pObjectManager->SetDocument(this);
  m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
  m_CommandHistory = EZ_DEFAULT_NEW(ezCommandHistory, this);
  m_CommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
  m_SelectionManager = EZ_DEFAULT_NEW(ezSelectionManager);
  m_SelectionManager->SetOwner(m_pObjectManager.Borrow());
  m_SelectionManager->SwapStorage(m_sceneSelectionManager->GetStorage());
  m_ObjectAccessor = EZ_DEFAULT_NEW(ezObjectCommandAccessor, m_CommandHistory.Borrow());

  UpdateLayers();
}

const ezDocumentObject* ezScene2Document::GetSettingsObject() const
{
  /// This function is overwritten so that after redirecting to the active document this still accesses the original content and is not redirected.
  if (m_pSceneObjectManager == nullptr)
    return SUPER::GetSettingsObject();

  auto pRoot = GetSceneObjectManager()->GetRootObject();
  ezVariant value;
  EZ_VERIFY(GetSceneObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  ezUuid id = value.Get<ezUuid>();
  return GetSceneObjectManager()->GetObject(id);
}

ezTaskGroupID ezScene2Document::InternalSaveDocument(AfterSaveCallback callback)
{
  // We need to switch the active layer back to the original content as otherwise the scene will not save itself but instead the active layer's content into itself.
  SetActiveLayer(GetGuid()).LogFailure();
  return SUPER::InternalSaveDocument(callback);
}

void ezScene2Document::LayerSelectionEventHandler(const ezSelectionManagerEvent& e)
{
  const ezDocumentObject* pObject = m_LayerSelection.GetCurrentObject();
  if (pObject)
  {
    if (pObject->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezSceneLayer>()))
    {
      ezUuid layerGuid = GetSceneObjectAccessor()->Get<ezUuid>(pObject, "Layer");
      if (IsLayerLoaded(layerGuid))
      {
        SetActiveLayer(layerGuid).LogFailure();
      }
    }
  }
}

void ezScene2Document::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
}

void ezScene2Document::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case ezCommandHistoryEvent::Type::UndoEnded:
    case ezCommandHistoryEvent::Type::RedoEnded:
    case ezCommandHistoryEvent::Type::TransactionEnded:
      UpdateLayers();
      break;
    default:
      return;
  }
}

void ezScene2Document::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentOpened:
    {
      if (e.m_pDocument->GetDynamicRTTI()->IsDerivedFrom<ezSceneDocument>())
      {
        ezUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        if (m_Layers.TryGetValue(layerGuid, pInfo))
        {
          pInfo->m_pLayer = static_cast<ezSceneDocument*>(e.m_pDocument);

          ezScene2LayerEvent e;
          e.m_Type = ezScene2LayerEvent::Type::LayerLoaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    case ezDocumentManager::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument->GetDynamicRTTI()->IsDerivedFrom<ezSceneDocument>())
      {
        ezUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        if (m_Layers.TryGetValue(layerGuid, pInfo))
        {
          pInfo->m_pLayer = nullptr;

          ezScene2LayerEvent e;
          e.m_Type = ezScene2LayerEvent::Type::LayerUnloaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
  }
}

void ezScene2Document::UpdateLayers()
{
  ezSet<ezUuid> layersBefore;
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    layersBefore.Insert(it.Key());
  }
  ezSet<ezUuid> layersAfter;
  ezMap<ezUuid, ezUuid> LayerToSceneObject;
  const ezSceneDocumentSettings* pSettings = GetSettings<ezSceneDocumentSettings>();
  for (const ezSceneLayerBase* pLayerBase : pSettings->m_Layers)
  {
    if (const ezSceneLayer* pLayer = ezDynamicCast<const ezSceneLayer*>(pLayerBase))
    {
      layersAfter.Insert(pLayer->m_Layer);
      ezUuid objectGuid = m_Context.GetObjectGUID(ezGetStaticRTTI<ezSceneLayer>(), pLayer);
      LayerToSceneObject.Insert(pLayer->m_Layer, objectGuid);
    }
  }

  ezSet<ezUuid> layersRemoved = layersBefore;
  layersRemoved.Difference(layersAfter);
  for (auto it = layersRemoved.GetIterator(); it.IsValid(); ++it)
  {
    LayerRemoved(it.Key());
  }

  ezSet<ezUuid> layersAdded = layersAfter;
  layersAdded.Difference(layersBefore);
  for (auto it = layersAdded.GetIterator(); it.IsValid(); ++it)
  {
    LayerAdded(it.Key(), LayerToSceneObject[it.Key()]);
  }
}

void ezScene2Document::LayerAdded(const ezUuid& layerGuid, const ezUuid& layerObjectGuid)
{
  LayerInfo info;
  info.m_pLayer = nullptr;
  info.m_bVisible = true;
  info.m_objectGuid = layerObjectGuid;
  m_Layers.Insert(layerGuid, info);

  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::LayerAdded;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  //#TODO Decide whether to load a layer or not (persist as meta data? / user preferences?)
  SetLayerLoaded(layerGuid, true).LogFailure();
}

void ezScene2Document::LayerRemoved(const ezUuid& layerGuid)
{
  // Make sure removed layer is not active
  if (m_ActiveLayerGuid == layerGuid)
  {
    SetActiveLayer(GetGuid()).LogFailure();
  }

  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::LayerRemoved;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  SetLayerLoaded(layerGuid, false).LogFailure();

  m_Layers.Remove(layerGuid);
}

ezStatus ezScene2Document::CreateLayer(const char* szName, const ezUuid& out_layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  ezStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  const ezDocumentTypeDescriptor* pLayerDesc = ezDocumentManager::GetDescriptorForDocumentType("Layer");

  ezStringBuilder targetDirectory = GetDocumentPath();
  targetDirectory.RemoveFileExtension();
  targetDirectory.Append("_data");
  targetDirectory.AppendPath(szName);
  targetDirectory.Append(".", pLayerDesc->m_sFileExtension.GetData());

  ezSceneDocument* pLayerDoc = nullptr;
  if (ezOSFile::ExistsFile(targetDirectory))
  {
    pLayerDoc = ezDynamicCast<ezSceneDocument*>(ezQtEditorApp::GetSingleton()->OpenDocument(targetDirectory, ezDocumentFlags::None));
    //auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset();


    //return ezStatus(ezFmt("The layer file '{}' already exists.", targetDirectory));
  }
  else
  {

    pLayerDoc = ezDynamicCast<ezSceneDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(targetDirectory, ezDocumentFlags::None));
    if (!pLayerDoc)
    {
      return ezStatus(ezFmt("Failed to create new layer '{0}'", targetDirectory));
    }
  }

  ezObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  ezStringBuilder sTransactionText;
  pAccessor->StartTransaction(ezFmt("Add Layer - '{}'", szName).GetText(sTransactionText));
  {
    auto pRoot = m_pSceneObjectManager->GetObject(GetSettingsObject()->GetGuid());
    ezInt32 uiCount = 0;
    EZ_VERIFY(pAccessor->GetCount(pRoot, "Layers", uiCount).Succeeded(), "Failed to get layer count.");
    ezUuid sceneLayerGuid;
    EZ_VERIFY(pAccessor->AddObject(pRoot, "Layers", uiCount, ezGetStaticRTTI<ezSceneLayer>(), sceneLayerGuid).Succeeded(), "Failed to add layer to scene.");
    auto pLayer = pAccessor->GetObject(sceneLayerGuid);
    EZ_VERIFY(pAccessor->SetValue(pLayer, "Layer", pLayerDoc->GetGuid()).Succeeded(), "Failed to set layer GUID.");
  }
  pAccessor->FinishTransaction();

  // We need to manually emit this here as when the layer doc was loaded DocumentManagerEventHandler will not fire as the document was not added as a layer yet.
  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::LayerLoaded;
  e.m_layerGuid = pLayerDoc->GetGuid();
  m_LayerEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezScene2Document::DeleteLayer(const ezUuid& layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  ezStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return ezStatus("Unknown layer guid. Layer can't be deleted.");
  }

  if (!pInfo->m_objectGuid.IsValid())
  {
    return ezStatus("Layer object guid not set, layer object unknown.");
  }

  const ezDocumentObject* pObject = GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  if (!pObject)
  {
    return ezStatus("Layer object no longer valid.");
  }

  ezStringBuilder sName("<Unknown>");
  {
    auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
    if (assetInfo.isValid())
    {
      sName = ezPathUtils::GetFileName(assetInfo->m_pAssetInfo->m_sDataDirRelativePath);
    }
    else
    {
      return ezStatus("Could not resolve layer in ezAssetCurator.");
    }
  }

  ezObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  ezStringBuilder sTransactionText;
  pAccessor->StartTransaction(ezFmt("Remove Layer - '{}'", sName).GetText(sTransactionText));
  {
    EZ_VERIFY(pAccessor->RemoveObject(pObject).Succeeded(), "Failed to remove Layer.");
  }
  pAccessor->FinishTransaction();
  return ezStatus(EZ_SUCCESS);
}

const ezUuid& ezScene2Document::GetActiveLayer() const
{
  return m_ActiveLayerGuid;
}

ezStatus ezScene2Document::SetActiveLayer(const ezUuid& layerGuid)
{
  if (layerGuid == m_ActiveLayerGuid)
    return ezStatus(EZ_SUCCESS);

  if (layerGuid == GetGuid())
  {
    ezDocumentObjectStructureEvent e;
    e.m_pDocument = m_pObjectManager->GetDocument();
    e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
    m_CommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
    m_SelectionManager->SwapStorage(m_sceneSelectionManager->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }
  else
  {
    ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(layerGuid);
    if (!pDoc)
      return ezStatus("Unladed layer can't be made active.");

    ezDocumentObjectStructureEvent e;
    e.m_pDocument = m_pObjectManager->GetDocument();
    e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(pDoc->GetObjectManager()->GetStorage());
    m_CommandHistory->SwapStorage(pDoc->GetCommandHistory()->GetStorage());
    m_SelectionManager->SwapStorage(pDoc->GetSelectionManager()->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }

  {
    ezSelectionManagerEvent se;
    se.m_pDocument = m_pObjectManager->GetDocument();
    se.m_pObject = nullptr;
    se.m_Type = ezSelectionManagerEvent::Type::SelectionSet;
    m_SelectionManager->GetStorage()->m_Events.Broadcast(se);
  }
  {
    ezCommandHistoryEvent ce;
    ce.m_pDocument = m_pObjectManager->GetDocument();
    ce.m_Type = ezCommandHistoryEvent::Type::HistoryChanged;
    m_CommandHistory->GetStorage()->m_Events.Broadcast(ce);
  }
  m_ActiveLayerGuid = layerGuid;
  {
    ezScene2LayerEvent e;
    e.m_Type = ezScene2LayerEvent::Type::ActiveLayerChanged;
    e.m_layerGuid = layerGuid;
    m_LayerEvents.Broadcast(e);
  }

  // Set selection to object that contains the active layer
  if (const ezDocumentObject* pLayerObject = GetLayerObject(layerGuid))
  {
    m_LayerSelection.SetSelection(pLayerObject);
  }
  return ezStatus(EZ_SUCCESS);
}

bool ezScene2Document::IsLayerLoaded(const ezUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer != nullptr;
  }
  return false;
}

ezStatus ezScene2Document::SetLayerLoaded(const ezUuid& layerGuid, bool bLoaded)
{
  // We can't unload the active layer
  if (!bLoaded && m_ActiveLayerGuid == layerGuid)
  {
    ezStatus res = SetActiveLayer(GetGuid());
    if (res.Failed())
      return res;
  }

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return ezStatus("Unknown layer guid. Layer can't be loaded / unloaded.");
  }

  if ((pInfo->m_pLayer != nullptr) == bLoaded)
    return ezStatus(EZ_SUCCESS);

  if (bLoaded)
  {
    ezStringBuilder sAbsPath;
    {
      auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (assetInfo.isValid())
      {
        sAbsPath = assetInfo->m_pAssetInfo->m_sAbsolutePath;
      }
      else
      {
        return ezStatus("Could not resolve layer in ezAssetCurator.");
      }
    }

    ezDocument* pDoc = nullptr;
    //#TODO Open layer. Pass our root into it to indicate what the parent context of the layer is.
    //#TODO context is a window-only concept and not passed to the doc.
    ezDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    if (ezDocument* pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, ezDocumentFlags::None, pRoot))
    {
      pInfo->m_pLayer = ezDynamicCast<ezSceneDocument*>(pDoc);

      ezScene2LayerEvent e;
      e.m_Type = ezScene2LayerEvent::Type::LayerLoaded;
      e.m_layerGuid = layerGuid;
      m_LayerEvents.Broadcast(e);

      return ezStatus(EZ_SUCCESS);
    }
    else
    {
      return ezStatus("Could not load layer, see log for more information.");
    }
  }
  else
  {
    if (pInfo->m_pLayer == nullptr)
      return ezStatus(EZ_SUCCESS);

    // Unload document (save and close)
    pInfo->m_pLayer->SaveDocument();
    ezDocumentManager* pManager = pInfo->m_pLayer->GetDocumentManager();
    pManager->CloseDocument(pInfo->m_pLayer);
    pInfo->m_pLayer = nullptr;

    ezScene2LayerEvent e;
    e.m_Type = ezScene2LayerEvent::Type::LayerUnloaded;
    e.m_layerGuid = layerGuid;
    m_LayerEvents.Broadcast(e);

    return ezStatus(EZ_SUCCESS);
  }
}

void ezScene2Document::GetLoadedLayers(ezDynamicArray<ezSceneDocument*>& out_LayerGuids) const
{
  out_LayerGuids.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLayer)
    {
      out_LayerGuids.PushBack(it.Value().m_pLayer);
    }
  }
}

const ezDocumentObject* ezScene2Document::GetLayerObject(const ezUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  }
  return nullptr;
}

ezSceneDocument* ezScene2Document::GetLayerDocument(const ezUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer;
  }
  return nullptr;
}
