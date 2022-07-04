#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
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
  m_LayerSelection = EZ_DEFAULT_NEW(ezSelectionManager, m_pObjectManager.Borrow());
}

ezScene2Document::~ezScene2Document()
{
  SetActiveLayer(GetGuid()).LogFailure();

  // We need to clear all things that are dependent in the current object manager, selection etc setup before we swap the managers as otherwise those will fail to de-register.
  ezVisualizerManager::GetSingleton()->SetVisualizersActive(this, false);
  m_SelectionManager->Clear();

  // Game object document subscribed to the true document originally but we rerouted that to the mock data.
  // In order to destroy the game object document we need to revert this and subscribe to the true document again.
  UnsubscribeGameObjectEventHandlers();

  // Move the preserved real scene document back.
  m_SelectionManager = std::move(m_sceneSelectionManager);
  m_CommandHistory = std::move(m_pSceneCommandHistory);
  m_pObjectManager = std::move(m_pSceneObjectManager);
  m_ObjectAccessor = std::move(m_pSceneObjectAccessor);
  m_DocumentObjectMetaData = std::move(m_SceneDocumentObjectMetaData);
  m_GameObjectMetaData = std::move(m_SceneGameObjectMetaData);

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  m_documentManagerEventSubscriber.Unsubscribe();
  m_layerSelectionEventSubscriber.Unsubscribe();
  m_structureEventSubscriber.Unsubscribe();
  m_commandHistoryEventSubscriber.Unsubscribe();

  m_LayerSelection = nullptr;

  for (auto it : m_Layers)
  {
    auto pDoc = it.Value().m_pLayer;

    if (pDoc && pDoc != this)
    {
      ezDocumentManager* pManager = pDoc->GetDocumentManager();
      pManager->CloseDocument(pDoc);
    }
  }
}

void ezScene2Document::InitializeAfterLoading(bool bFirstTimeCreation)
{
  EnsureSettingsObjectExist();

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
  m_LayerSelection->m_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::LayerSelectionEventHandler, this), m_layerSelectionEventSubscriber);
  m_pObjectManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezScene2Document::StructureEventHandler, this), m_structureEventSubscriber);
  m_CommandHistory->m_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::CommandHistoryEventHandler, this), m_commandHistoryEventSubscriber);
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezScene2Document::DocumentManagerEventHandler, this), m_documentManagerEventSubscriber);

  SUPER::InitializeAfterLoadingAndSaving();

  // Game object document subscribed to the true document originally but want to reroute it to the mock data so that is picks up the layer content on its own.
  // Therefore we need to unsubscribe the original subscriptions and replace them with ones to the mock data.
  UnsubscribeGameObjectEventHandlers();

  // These preserve the real scene document.
  m_pSceneObjectManager = std::move(m_pObjectManager);
  m_pSceneCommandHistory = std::move(m_CommandHistory);
  m_sceneSelectionManager = std::move(m_SelectionManager);
  m_pSceneObjectAccessor = std::move(m_ObjectAccessor);
  m_SceneDocumentObjectMetaData = std::move(m_DocumentObjectMetaData);
  m_SceneGameObjectMetaData = std::move(m_GameObjectMetaData);

  // Replace real scene elements with copies.
  m_pObjectManager = EZ_DEFAULT_NEW(ezSceneObjectManager);
  m_pObjectManager->SetDocument(this);
  m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
  m_CommandHistory = EZ_DEFAULT_NEW(ezCommandHistory, this);
  m_CommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
  m_SelectionManager = EZ_DEFAULT_NEW(ezSelectionManager, m_pSceneObjectManager.Borrow());
  m_SelectionManager->SwapStorage(m_sceneSelectionManager->GetStorage());
  m_ObjectAccessor = EZ_DEFAULT_NEW(ezObjectCommandAccessor, m_CommandHistory.Borrow());
  using ObjectMetaData = ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>;
  m_DocumentObjectMetaData = EZ_DEFAULT_NEW(ObjectMetaData);
  m_DocumentObjectMetaData->SwapStorage(m_SceneDocumentObjectMetaData->GetStorage());
  using GameObjectMetaData = ezObjectMetaData<ezUuid, ezGameObjectMetaData>;
  m_GameObjectMetaData = EZ_DEFAULT_NEW(GameObjectMetaData);
  m_GameObjectMetaData->SwapStorage(m_SceneGameObjectMetaData->GetStorage());

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  UpdateLayers();
  if (const ezDocumentObject* pLayerObject = GetLayerObject(GetActiveLayer()))
  {
    m_LayerSelection->SetSelection(pLayerObject);
  }
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

void ezScene2Document::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (const ezPushObjectStateMsgToEditor* msg = ezDynamicCast<const ezPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg2(msg);
    return;
  }

  SUPER::HandleEngineMessage(pMsg);
}

ezTaskGroupID ezScene2Document::InternalSaveDocument(AfterSaveCallback callback)
{
  // We need to switch the active layer back to the original content as otherwise the scene will not save itself but instead the active layer's content into itself.
  SetActiveLayer(GetGuid()).LogFailure();
  return SUPER::InternalSaveDocument(callback);
}

void ezScene2Document::SendGameWorldToEngine()
{
  SUPER::SendGameWorldToEngine();
  for (auto layer : m_Layers)
  {
    ezSceneDocument* pLayer = layer.Value().m_pLayer;
    if (pLayer != this && pLayer != nullptr)
    {
      ezEditorEngineProcessConnection::GetSingleton()->SendDocumentOpenMessage(pLayer, true);
    }
  }
}

void ezScene2Document::LayerSelectionEventHandler(const ezSelectionManagerEvent& e)
{
  const ezDocumentObject* pObject = m_LayerSelection->GetCurrentObject();
  // We can't change the active layer while a transaction is in progress at it will swap out the data storage the transaction is currently modifying.
  if (pObject && !m_CommandHistory->IsInTransaction() && !m_pSceneCommandHistory->IsInTransaction())
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
      if (ezLayerDocument* pLayer = ezDynamicCast<ezLayerDocument*>(e.m_pDocument))
      {
        if (pLayer->GetMainDocument() != this)
          return;

        ezUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        // Either the layer is currently being creating, in which case m_Layers can't be filled yet,
        // or an existing layer's state is toggled either internally by the scene or externally by the editor in which case the layer is known and we must react to it.
        if (m_Layers.TryGetValue(layerGuid, pInfo) && pInfo->m_pLayer != pLayer)
        {
          pInfo->m_pLayer = pLayer;

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
      if (e.m_pDocument->GetDynamicRTTI()->IsDerivedFrom<ezLayerDocument>())
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

void ezScene2Document::HandleObjectStateFromEngineMsg2(const ezPushObjectStateMsgToEditor* pMsg)
{
  ezMap<ezUuid, ezHybridArray<const ezPushObjectStateData*, 1>> layerToChanges;
  for (const ezPushObjectStateData& change : pMsg->m_ObjectStates)
  {
    layerToChanges[change.m_LayerGuid].PushBack(&change);
  }

  const ezUuid activeLayer = m_ActiveLayerGuid;
  for (auto it : layerToChanges)
  {
    if (SetActiveLayer(it.Key()).Failed())
      continue;

    auto pHistory = GetCommandHistory();

    pHistory->StartTransaction("Pull Object State");

    for (const ezPushObjectStateData* pState : it.Value())
    {
      auto pObject = GetObjectManager()->GetObject(pState->m_ObjectGuid);

      if (!pObject)
        continue;

      // set the general transform of the object
      SetGlobalTransform(pObject, ezTransform(pState->m_vPosition, pState->m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);

      // if we also have bone transforms, attempt to set them as well
      if (pState->m_BoneTransforms.IsEmpty())
        continue;

      auto pAccessor = GetObjectAccessor();

      // check all components
      for (auto pComponent : pObject->GetChildren())
      {
        auto pComponentType = pComponent->GetType();

        const auto* pBoneManipAttr = pComponentType->GetAttributeByType<ezBoneManipulatorAttribute>();

        // we can only apply bone transforms on components that have the ezBoneManipulatorAttribute attribute
        if (pBoneManipAttr == nullptr)
          continue;

        auto pBonesProperty = pComponentType->FindPropertyByName(pBoneManipAttr->GetTransformProperty());
        EZ_ASSERT_DEBUG(pBonesProperty, "Invalid transform property set on ezBoneManipulatorAttribute");

        const ezExposedParametersAttribute* pExposedParamsAttr = pBonesProperty->GetAttributeByType<ezExposedParametersAttribute>();
        EZ_ASSERT_DEBUG(pExposedParamsAttr, "Expected exposed parameters on ezBoneManipulatorAttribute property");

        const ezAbstractProperty* pParameterSourceProp = pComponentType->FindPropertyByName(pExposedParamsAttr->GetParametersSource());
        EZ_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pExposedParamsAttr->GetParametersSource(), pComponentType->GetTypeName());

        // retrieve all the bone keys and values, these will contain the exposed default values, in case a bone has never been overridden before
        ezVariantArray boneValues, boneKeys;
        ezExposedParameterCommandAccessor proxy(pAccessor, pBonesProperty, pParameterSourceProp);
        proxy.GetValues(pComponent, pBonesProperty, boneValues);
        proxy.GetKeys(pComponent, pBonesProperty, boneKeys);

        // apply all the new bone transforms
        for (const auto& bone : pState->m_BoneTransforms)
        {
          // ignore bones that are unknown (not exposed somehow)
          ezUInt32 idx = boneKeys.IndexOf(bone.Key());
          if (idx == ezInvalidIndex)
            continue;

          EZ_ASSERT_DEBUG(boneValues[idx].GetReflectedType() == ezGetStaticRTTI<ezExposedBone>(), "Expected an ezExposedBone in variant");

          // retrieve the default/previous value of the bone
          const ezExposedBone* pDefVal = reinterpret_cast<const ezExposedBone*>(boneValues[idx].GetData());

          ezExposedBone b;
          b.m_sName = pDefVal->m_sName;     // same as the key
          b.m_sParent = pDefVal->m_sParent; // this is what we don't have and therefore needed to retrieve the default values
          b.m_Transform = bone.Value();

          ezVariant var;
          var.CopyTypedObject(&b, ezGetStaticRTTI<ezExposedBone>());

          proxy.SetValue(pComponent, pBonesProperty, var, bone.Key());
        }

        // found a component/property to apply bones to, so we can stop
        break;
      }
    }

    pHistory->FinishTransaction();
  }
  SetActiveLayer(activeLayer);
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

void ezScene2Document::SendLayerVisibility()
{
  ezLayerVisibilityChangedMsgToEngine msg;
  for (auto& layer : m_Layers)
  {
    if (!layer.Value().m_bVisible)
    {
      // We are sending the hidden state because the default state is visible so we have less to send and less often.
      msg.m_HiddenLayers.PushBack(layer.Key());
    }
  }
  SendMessageToEngine(&msg);
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

  SetLayerLoaded(layerGuid, false).LogFailure();

  ezScene2LayerEvent e;
  e.m_Type = ezScene2LayerEvent::Type::LayerRemoved;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  m_Layers.Remove(layerGuid);
}

ezStatus ezScene2Document::CreateLayer(const char* szName, ezUuid& out_layerGuid)
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
    ezDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc = ezDynamicCast<ezSceneDocument*>(ezQtEditorApp::GetSingleton()->OpenDocument(targetDirectory, ezDocumentFlags::None, pRoot));

    if (m_Layers.Contains(pLayerDoc->GetGuid()))
    {
      return ezStatus(ezFmt("A layer named '{}' already exists in this scene.", szName));
    }
  }
  else
  {
    ezDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc = ezDynamicCast<ezSceneDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(targetDirectory, ezDocumentFlags::None, pRoot));
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

  LayerInfo* pInfo = nullptr;
  EZ_ASSERT_DEV(m_Layers.Contains(pLayerDoc->GetGuid()), "FinishTransaction should have triggered UpdateLayers and filled m_Layers.");
  // We need to manually emit this here as when the layer doc was loaded DocumentManagerEventHandler will not fire as the document was not added as a layer yet.
  if (m_Layers.TryGetValue(pLayerDoc->GetGuid(), pInfo) && pInfo->m_pLayer != pLayerDoc)
  {
    pInfo->m_pLayer = pLayerDoc;

    ezScene2LayerEvent e;
    e.m_Type = ezScene2LayerEvent::Type::LayerLoaded;
    e.m_layerGuid = pLayerDoc->GetGuid();
    m_LayerEvents.Broadcast(e);
  }
  out_layerGuid = pLayerDoc->GetGuid();
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
      sName = ezPathUtils::GetFileName(assetInfo->m_pAssetInfo->m_sDataDirParentRelativePath);
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
  EZ_ASSERT_DEV(!m_CommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");
  EZ_ASSERT_DEV(!m_pSceneCommandHistory || !m_pSceneCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");

  if (layerGuid == m_ActiveLayerGuid)
    return ezStatus(EZ_SUCCESS);

  if (layerGuid == GetGuid())
  {
    ezDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
    m_CommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
    m_SelectionManager->SwapStorage(m_sceneSelectionManager->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(m_SceneDocumentObjectMetaData->GetStorage());
    m_GameObjectMetaData->SwapStorage(m_SceneGameObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }
  else
  {
    ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(layerGuid);
    if (!pDoc)
      return ezStatus("Unloaded layer can't be made active.");

    ezDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(pDoc->GetObjectManager()->GetStorage());
    m_CommandHistory->SwapStorage(pDoc->GetCommandHistory()->GetStorage());
    m_SelectionManager->SwapStorage(pDoc->GetSelectionManager()->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(pDoc->m_DocumentObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }

  {
    ezSelectionManagerEvent se;
    se.m_pDocument = this;
    se.m_pObject = nullptr;
    se.m_Type = ezSelectionManagerEvent::Type::SelectionSet;
    m_SelectionManager->GetStorage()->m_Events.Broadcast(se);
  }
  {
    ezCommandHistoryEvent ce;
    ce.m_pDocument = this;
    ce.m_Type = ezCommandHistoryEvent::Type::HistoryChanged;
    m_CommandHistory->GetStorage()->m_Events.Broadcast(ce);
  }
  if (m_ActiveLayerGuid != GetGuid())
  {
    ezVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), false);
  }
  m_ActiveLayerGuid = layerGuid;
  m_pActiveSubDocument = GetLayerDocument(layerGuid);
  {
    ezScene2LayerEvent e;
    e.m_Type = ezScene2LayerEvent::Type::ActiveLayerChanged;
    e.m_layerGuid = layerGuid;
    m_LayerEvents.Broadcast(e);
  }
  {
    ezDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = ezDocumentEvent::Type::ModifiedChanged;

    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);
  }
  {
    ezActiveLayerChangedMsgToEngine msg;
    msg.m_ActiveLayer = layerGuid;
    SendMessageToEngine(&msg);
  }
  if (m_ActiveLayerGuid != GetGuid())
  {
    ezVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), true);
  }

  // Set selection to object that contains the active layer
  if (const ezDocumentObject* pLayerObject = GetLayerObject(layerGuid))
  {
    m_LayerSelection->SetSelection(pLayerObject);
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
  if (GetGameMode() != GameMode::Enum::Off)
    return ezStatus("Simulation must be stopped to change a layer's loaded state.");

  if (layerGuid == GetGuid() && !bLoaded)
    return ezStatus("Cannot unload the scene itself.");

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
    if (layerGuid == GetGuid())
    {
      sAbsPath = GetDocumentPath();
    }
    else
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
    // Pass our root into it to indicate what the parent context of the layer is.
    ezDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    if (ezDocument* pLayer = ezQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, ezDocumentFlags::None, pRoot))
    {
      if (layerGuid != GetGuid() && pLayer->GetMainDocument() != this)
      {
        return ezStatus("Layer already open in another window.");
      }

      // In case we are responding to e.g. an redo 'Add Layer' the layer is already loaded in the editor but we still want to enforce that the event is fired every time after adding a layer.
      if (pInfo->m_pLayer != pLayer)
      {
        pInfo->m_pLayer = ezDynamicCast<ezSceneDocument*>(pLayer);

        ezScene2LayerEvent e;
        e.m_Type = ezScene2LayerEvent::Type::LayerLoaded;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }

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
    ezDocumentManager* pManager = pInfo->m_pLayer->GetDocumentManager();
    pManager->CloseDocument(pInfo->m_pLayer);
    pInfo->m_pLayer = nullptr;

    //ezScene2LayerEvent e;
    //e.m_Type = ezScene2LayerEvent::Type::LayerUnloaded;
    //e.m_layerGuid = layerGuid;
    //m_LayerEvents.Broadcast(e);

    return ezStatus(EZ_SUCCESS);
  }
}

void ezScene2Document::GetAllLayers(ezDynamicArray<ezUuid>& out_LayerGuids)
{
  out_LayerGuids.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    out_LayerGuids.PushBack(it.Key());
  }
}

void ezScene2Document::GetLoadedLayers(ezDynamicArray<ezSceneDocument*>& out_Layers) const
{
  out_Layers.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLayer)
    {
      out_Layers.PushBack(it.Value().m_pLayer);
    }
  }
}

bool ezScene2Document::IsLayerVisible(const ezUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_bVisible;
  }
  return false;
}

ezStatus ezScene2Document::SetLayerVisible(const ezUuid& layerGuid, bool bVisible)
{
  LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    if (pInfo->m_bVisible != bVisible)
    {
      pInfo->m_bVisible = bVisible;
      {
        ezScene2LayerEvent e;
        e.m_Type = bVisible ? ezScene2LayerEvent::Type::LayerVisible : ezScene2LayerEvent::Type::LayerInvisible;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }
      SendLayerVisibility();
    }
    return ezStatus(EZ_SUCCESS);
  }
  return ezStatus("Unknown layer.");
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

bool ezScene2Document::IsAnyLayerModified() const
{
  for (auto& layer : m_Layers)
  {
    auto pLayer = layer.Value().m_pLayer;
    if (pLayer && pLayer->IsModified())
      return true;
  }

  return false;
}
