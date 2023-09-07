#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetDocument, 5, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

bool ezRenderPipelineNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<ezRenderPipelinePass>() || pType->IsDerivedFrom<ezExtractor>();
}

void ezRenderPipelineNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<ezRenderPipelinePass>())
    return;

  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodePin>())
      continue;

    ezColor pinColor;
    if (const ezColorAttribute* pAttr = pProp->GetAttributeByType<ezColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }
    else
    {
      ezColorScheme::Enum color = ezColorScheme::Gray;
      if (ezStringUtils::IsEqual(pProp->GetPropertyName(), "DepthStencil"))
        color = ezColorScheme::Pink;

      pinColor = ezColorScheme::DarkUI(color);
    }

    if (pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodeInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodeOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodePassThrougPin>())
    {
      auto pPinIn = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPinIn);
      auto pPinOut = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void ezRenderPipelineNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  ezSet<const ezRTTI*> typeSet;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezRenderPipelinePass>(), typeSet, false);
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezExtractor>(), typeSet, false);
  ref_types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

ezStatus ezRenderPipelineNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNto1;
  return ezStatus(EZ_SUCCESS);
}

ezRenderPipelineAssetDocument::ezRenderPipelineAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezRenderPipelineNodeManager), ezAssetDocEngineConnection::FullObjectMirroring)
{
}

ezRenderPipelineAssetDocument::~ezRenderPipelineAssetDocument()
{
  static_cast<ezRenderPipelineObjectMirrorEditor*>(m_pMirror.Borrow())->DeInitNodeSender();
}


void ezRenderPipelineAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = EZ_DEFAULT_NEW(ezRenderPipelineObjectMirrorEditor);
  static_cast<ezRenderPipelineObjectMirrorEditor*>(m_pMirror.Borrow())->InitNodeSender(static_cast<const ezDocumentNodeManager*>(GetObjectManager()));
}

ezTransformStatus ezRenderPipelineAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  return ezAssetDocument::RemoteExport(AssetHeader, szTargetFile);
}

ezTransformStatus ezRenderPipelineAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_REPORT_FAILURE("Should not be called");
  return ezTransformStatus();
}

void ezRenderPipelineAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void ezRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.RenderPipelineGraph");
}

bool ezRenderPipelineAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.RenderPipelineGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezRenderPipelineAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

void ezRenderPipelineObjectMirrorEditor::InitNodeSender(const ezDocumentNodeManager* pNodeManager)
{
  m_pNodeManager = pNodeManager;
  m_pNodeManager->m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezRenderPipelineObjectMirrorEditor::NodeEventsHandler, this));
}

void ezRenderPipelineObjectMirrorEditor::DeInitNodeSender()
{
  m_pNodeManager->m_NodeEvents.RemoveEventHandler(ezMakeDelegate(&ezRenderPipelineObjectMirrorEditor::NodeEventsHandler, this));
}

void ezRenderPipelineObjectMirrorEditor::ApplyOp(ezObjectChange& ref_change)
{
  // SUPER::ApplyOp will move the data out of the payload, so we have to check for connections before.
  const ezConnection* pConnection = nullptr;
  if (ref_change.m_Change.m_Operation == ezObjectChangeType::NodeAdded)
  {
    const ezDocumentObject* pObject = m_pNodeManager->GetObject(ref_change.m_Change.m_Value.Get<ezUuid>());
    if (pObject != nullptr && m_pNodeManager->IsConnection(pObject))
    {
      pConnection = &m_pNodeManager->GetConnection(pObject);
    }
  }
  SUPER::ApplyOp(ref_change);

  // We need to handle this case in addition to the NodeEventsHandler because after loading the meta data is restored before the object mirror is initialized so we miss all the NodeEventsHandler calls.
  if (pConnection)
    SendConnection(*pConnection);
}

void ezRenderPipelineObjectMirrorEditor::NodeEventsHandler(const ezDocumentNodeManagerEvent& e)
{
  if (e.m_EventType == ezDocumentNodeManagerEvent::Type::AfterPinsConnected)
  {
    const ezConnection& connection = m_pNodeManager->GetConnection(e.m_pObject);
    SendConnection(connection);
  }
}

void ezRenderPipelineObjectMirrorEditor::SendConnection(const ezConnection& connection)
{
  const ezPin& sourcePin = connection.GetSourcePin();
  const ezPin& targetPin = connection.GetTargetPin();

  ezUuid Source = sourcePin.GetParent()->GetGuid();
  ezUuid Target = targetPin.GetParent()->GetGuid();
  ezString SourcePin = sourcePin.GetName();
  ezString TargetPin = targetPin.GetName();

  auto SendMetaData = [this](const ezDocumentObject* pObject, const char* szProperty, ezVariant value) {
    ezObjectChange change;
    CreatePath(change, pObject, szProperty);
    change.m_Change.m_Operation = ezObjectChangeType::PropertySet;
    change.m_Change.m_Value = value;
    ApplyOp(change);
  };
  SendMetaData(connection.GetParent(), "Connection::Source", Source);
  SendMetaData(connection.GetParent(), "Connection::Target", Target);
  SendMetaData(connection.GetParent(), "Connection::SourcePin", SourcePin);
  SendMetaData(connection.GetParent(), "Connection::TargetPin", TargetPin);
}
