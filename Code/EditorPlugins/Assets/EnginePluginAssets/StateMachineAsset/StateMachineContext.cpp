#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/StateMachineAsset/StateMachineContext.h>
#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

#include <GameEngine/StateMachine/StateMachineBuiltins.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineContext, 1, ezRTTIDefaultAllocator<ezStateMachineContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "StateMachine"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineContext::ezStateMachineContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

ezEngineProcessViewContext* ezStateMachineContext::CreateViewContext()
{
  EZ_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void ezStateMachineContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_ASSERT_DEV(false, "Should not be called");
}

ezStatus ezStateMachineContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezDynamicArray<ezUuid> nodeUuids;
  ezDynamicArray<ezStateMachineNodeBase*> nodes;
  ezDynamicArray<ezStateMachineConnection*> connections;

  m_Context.GetObjectsByType(nodes, &nodeUuids);
  m_Context.GetObjectsByType(connections);

  ezStateMachineDescription desc;
  ezHashTable<ezUuid, ezUInt32> nodeUuidToStateIndex;
  ezSet<ezString> stateNames;

  auto AddState = [&](const ezStateMachineNode* pNode, const ezUuid& uuid)
  {
    const ezString& name = pNode->m_sName;
    if (stateNames.Contains(name))
    {
      return ezStatus(ezFmt("A state named '{}' already exists. State names have to be unique.", name));
    }
    stateNames.Insert(name);

    ezUniquePtr<ezStateMachineState> pState = ezUniquePtr<ezStateMachineState>(pNode->m_pType, nullptr);
    if (pState == nullptr)
    {
      pState = EZ_DEFAULT_NEW(ezStateMachineState_Empty);
    }

    if (pState->GetName().IsEmpty())
    {
      pState->SetName(name);
    }

    const ezUInt32 uiStateIndex = desc.AddState(std::move(pState));
    nodeUuidToStateIndex.Insert(uuid, uiStateIndex);

    return ezStatus(EZ_SUCCESS);
  };

  for (ezUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    auto pNode = ezDynamicCast<ezStateMachineNode*>(nodes[i]);
    if (pNode != nullptr && pNode->m_bIsInitialState)
    {
      const ezUuid& nodeUuid = nodeUuids[i];
      EZ_SUCCEED_OR_RETURN(AddState(pNode, nodeUuid));
      EZ_ASSERT_DEV(nodeUuidToStateIndex[nodeUuid] == 0, "Initial state has to have index 0");
      break;
    }
  }

  if (nodeUuidToStateIndex.IsEmpty())
  {
    return ezStatus("Initial state is not set");
  }

  for (ezUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    auto pNode = ezDynamicCast<ezStateMachineNode*>(nodes[i]);
    if (pNode == nullptr || pNode->m_bIsInitialState)
      continue;

    EZ_SUCCEED_OR_RETURN(AddState(pNode, nodeUuids[i]));
  }

  for (auto pConnection : connections)
  {
    ezUniquePtr<ezStateMachineTransition> pTransition = ezUniquePtr<ezStateMachineTransition>(pConnection->m_pType, nullptr);
    if (pTransition == nullptr)
    {
      pTransition = EZ_DEFAULT_NEW(ezStateMachineTransition_Timeout);
    }

    ezUInt32 uiFromStateIndex = ezInvalidIndex;
    ezUInt32 uiToStateIndex = ezInvalidIndex;
    nodeUuidToStateIndex.TryGetValue(pConnection->m_Source, uiFromStateIndex); // Can fail for any states
    EZ_VERIFY(nodeUuidToStateIndex.TryGetValue(pConnection->m_Target, uiToStateIndex), "Implementation error");

    desc.AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
  }

  ezDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // Asset Header
  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).IgnoreResult();
  }

  EZ_SUCCEED_OR_RETURN(desc.Serialize(file));

  // do the actual file writing
  if (file.Close().Failed())
    return ezStatus(ezFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return ezStatus(EZ_SUCCESS);
}
