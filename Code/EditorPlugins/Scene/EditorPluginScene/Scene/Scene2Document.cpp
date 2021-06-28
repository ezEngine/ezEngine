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
  m_LayerSelection.m_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::LayerSelectionEventHandler, this), m_layerSelectionEventSubscriber);

  m_pObjectManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezScene2Document::StructureEventHandler, this), m_structureEventSubscriber);
  m_CommandHistory->m_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::CommandHistoryEventHandler, this), m_commandHistoryEventSubscriber);
}

ezScene2Document::~ezScene2Document()
{
  SetActiveLayer(GetGuid());

  m_SelectionManager->SetOwner(nullptr);
  // Move the preserved real scene document back.
  m_pObjectManager = std::move(m_pSceneObjectManager);
  m_CommandHistory = std::move(m_pSceneCommandHistory);
  m_SelectionManager = std::move(m_sceneSelectionManager);
  m_ObjectAccessor = std::move(m_pSceneObjectAccessor);

  m_layerSelectionEventSubscriber.Clear();
  m_structureEventSubscriber.Clear();
  m_commandHistoryEventSubscriber.Clear();
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
  if (m_pSceneObjectManager == nullptr)
    return SUPER::GetSettingsObject();

  auto pRoot = GetSceneObjectManager()->GetRootObject();
  ezVariant value;
  EZ_VERIFY(GetSceneObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  ezUuid id = value.Get<ezUuid>();
  return GetSceneObjectManager()->GetObject(id);
}

void ezScene2Document::LayerSelectionEventHandler(const ezSelectionManagerEvent& e)
{
  const ezDocumentObject* pObject = m_LayerSelection.GetCurrentObject();
  if (pObject)
  {
    if (pObject->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezSceneLayer>()))
    {
      ezUuid layerGuid = GetSceneObjectAccessor()->Get<ezUuid>(pObject, "Layer");
      SetActiveLayer(layerGuid);
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

void ezScene2Document::UpdateLayers()
{
  ezSet<ezUuid> layersBefore;
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    layersBefore.Insert(it.Key());
  }
  ezSet<ezUuid> layersAfter;
  const ezSceneDocumentSettings* pSettings = GetSettings<ezSceneDocumentSettings>();
  for (const ezSceneLayerBase* pLayerBase : pSettings->m_Layers)
  {
    if (const ezSceneLayer* pLayer = ezDynamicCast<const ezSceneLayer*>(pLayerBase))
    {
      layersAfter.Insert(pLayer->m_Layer);
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
    LayerAdded(it.Key());
  }
}

void ezScene2Document::LayerAdded(const ezUuid& layerGuid)
{
  LayerInfo info;
  info.m_pLayer = nullptr;
  info.m_bVisible = true;
  m_Layers.Insert(layerGuid, info);

  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::LayerAdded;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  ezStringBuilder sAbsPath;
  {
    auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
    if (assetInfo.isValid())
    {
      sAbsPath = assetInfo->m_pAssetInfo->m_sAbsolutePath;
    }
  }
  ezDocument* pDoc = nullptr;

  ezDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
  ezQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, ezDocumentFlags::None, pRoot);

 // ezDocumentManager::OpenDocument()
  //#TODO Load document

  //#TODO Set to active
}

void ezScene2Document::LayerRemoved(const ezUuid& layerGuid)
{
  // Make sure removed layer is not active
  if (m_ActiveLayerGuid == layerGuid)
  {
    SetActiveLayer(GetGuid());
  }

  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::LayerRemoved;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  m_Layers.Remove(layerGuid);

  //#TODO Unload document
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
  //ezSceneDocumentSettings* pSettings = static_cast<ezSceneDocumentSettings*>(m_ObjectMirror.GetNativeObjectPointer(GetSettingsObject()));

  ezObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  ezStringBuilder sTransactionText;
  pAccessor->StartTransaction(ezFmt("Add Layer - '{}'", szName).GetText(sTransactionText));
  {
    auto pRoot = m_pSceneObjectManager->GetObject(GetSettingsObject()->GetGuid());
    //ezDocumentObject* pObject = m_pSceneObjectManager->CreateObject(ezGetStaticRTTI<ezSceneLayer>());
    ezInt32 uiCount = 0;
    EZ_VERIFY(pAccessor->GetCount(pRoot, "Layers", uiCount).Succeeded(), "Failed to get layer count.");
    ezUuid sceneLayerGuid;
    EZ_VERIFY(pAccessor->AddObject(pRoot, "Layers", uiCount, ezGetStaticRTTI<ezSceneLayer>(), sceneLayerGuid).Succeeded(), "Failed to add layer to scene.");
    auto pLayer = pAccessor->GetObject(sceneLayerGuid);
    EZ_VERIFY(pAccessor->SetValue(pLayer, "Layer", pLayerDoc->GetGuid()).Succeeded(), "Failed to set layer GUID.");
  }
  pAccessor->FinishTransaction();
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezScene2Document::DeleteLayer(const ezUuid& layerGuid)
{
  return ezStatus("FAIL");
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

  m_ActiveLayerGuid = layerGuid;

  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::ActiveLayerChanged;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

bool ezScene2Document::IsLayerLoaded(const ezUuid& layerGuid)
{
  return false;
}

ezStatus ezScene2Document::SetLayerLoaded(const ezUuid& layerGuid, bool bLoaded)
{
  return ezStatus("FAIL");
}
