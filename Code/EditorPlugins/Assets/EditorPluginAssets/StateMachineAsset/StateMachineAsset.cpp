#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineAssetDocument, 3, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineAssetDocument::ezStateMachineAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezStateMachineNodeManager), ezAssetDocEngineConnection::None)
{
}

ezStatus ezStateMachineAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  auto pManager = static_cast<ezStateMachineNodeManager*>(GetObjectManager());

  ezAbstractObjectGraph abstractObjectGraph;
  ezDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, pManager);
  ezRttiConverterContext converterContext;
  ezRttiConverterReader converter(&abstractObjectGraph, &converterContext);

  ezStateMachineDescription desc;
  ezHashTable<const ezDocumentObject*, ezUInt32> objectToStateIndex;
  ezSet<ezString> stateNames;

  auto AddState = [&](const ezDocumentObject* pObject) {
    ezVariant nameVar = pObject->GetTypeAccessor().GetValue("Name");
    EZ_ASSERT_DEV(nameVar.IsA<ezString>(), "Implementation error");

    const ezString& name = nameVar.Get<ezString>();
    if (stateNames.Contains(name))
    {
      return ezStatus(ezFmt("A state named '{}' already exists. State names have to be unique.", name));
    }
    stateNames.Insert(name);

    ezVariant type = pObject->GetTypeAccessor().GetValue("Type");
    EZ_ASSERT_DEV(type.IsA<ezUuid>(), "Implementation error");

    if (auto pStateObject = pObject->GetChild(type.Get<ezUuid>()))
    {
      ezAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pStateObject);
      auto pState = converter.CreateObjectFromNode(pAbstractNode).Cast<ezStateMachineState>();
      pState->SetName(name);

      const ezUInt32 uiStateIndex = desc.AddState(pState);
      objectToStateIndex.Insert(pObject, uiStateIndex);
    }
    else
    {
      return ezStatus(ezFmt("State '{}' has no state type assigned", name));
    }

    return ezStatus(EZ_SUCCESS);
  };

  auto& allObjects = pManager->GetRootObject()->GetChildren();

  if (allObjects.IsEmpty() == false)
  {
    if (auto pObject = pManager->GetInitialState())
    {
      EZ_SUCCEED_OR_RETURN(AddState(pObject));
      EZ_ASSERT_DEV(objectToStateIndex[pObject] == 0, "Initial state has to have index 0");
    }
    else
    {
      return ezStatus("Initial state is not set");
    }
  }

  for (const ezDocumentObject* pObject : allObjects)
  {
    if (pManager->IsNode(pObject) == false || pManager->IsInitialState(pObject) || pManager->IsAnyState(pObject))
      continue;

    EZ_SUCCEED_OR_RETURN(AddState(pObject));
  }

  for (const ezDocumentObject* pObject : allObjects)
  {
    if (pManager->IsConnection(pObject) == false)
      continue;

    ezVariant type = pObject->GetTypeAccessor().GetValue("Type");
    EZ_ASSERT_DEV(type.IsA<ezUuid>(), "Implementation error");

    ezUniquePtr<ezStateMachineTransition> pTransition;
    if (auto pTransitionObject = pObject->GetChild(type.Get<ezUuid>()))
    {
      ezAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pTransitionObject);
      pTransition = converter.CreateObjectFromNode(pAbstractNode).Cast<ezStateMachineTransition>();
    }
    else
    {
      pTransition = EZ_DEFAULT_NEW(ezStateMachineTransition_Timeout);
    }

    const ezConnection& connection = pManager->GetConnection(pObject);
    ezUInt32 uiFromStateIndex = ezInvalidIndex;
    ezUInt32 uiToStateIndex = ezInvalidIndex;
    if (pManager->IsAnyState(connection.GetSourcePin().GetParent()) == false)
    {
      EZ_VERIFY(objectToStateIndex.TryGetValue(connection.GetSourcePin().GetParent(), uiFromStateIndex), "Implementation error");
    }
    EZ_VERIFY(objectToStateIndex.TryGetValue(connection.GetTargetPin().GetParent(), uiToStateIndex), "Implementation error");

    desc.AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  EZ_SUCCEED_OR_RETURN(desc.Serialize(stream));

  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
}

constexpr const char* s_szIsInitialState = "IsInitialState";

void ezStateMachineAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const ezStateMachineNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);

  if (pManager->IsInitialState(pObject))
  {
    inout_uiHash = ezHashingUtils::xxHash64String(s_szIsInitialState, inout_uiHash);
  }
}

void ezStateMachineAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const ezStateMachineNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);

  if (auto pObject = pManager->GetInitialState())
  {
    ezAbstractObjectNode* pAbstractObject = graph.GetNode(pObject->GetGuid());
    pAbstractObject->AddProperty(s_szIsInitialState, true);
  }
}

void ezStateMachineAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<ezStateMachineNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);

  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    if (auto pProperty = pAbstractObject->FindProperty(s_szIsInitialState))
    {
      if (pProperty->m_Value.ConvertTo<bool>() == false)
        continue;

      ezDocumentObject* pObject = pManager->GetObject(pAbstractObject->GetGuid());
      pManager->SetInitialState(pObject);
    }
  }
}

void ezStateMachineAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.StateMachineGraph");
}

bool ezStateMachineAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.StateMachineGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezStateMachineAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
