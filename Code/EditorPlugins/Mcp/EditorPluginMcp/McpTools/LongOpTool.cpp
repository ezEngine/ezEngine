#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/LongOpTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpLongOpTool, 1, ezRTTIDefaultAllocator<ezMcpLongOpTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// A long op that hasn't finished by then is reported as still running rather than waited for further.
  constexpr ezTime TimeOut = ezTime::MakeFromMinutes(10);

  /// Writes what identifies one operation: which document and component it belongs to, and its state.
  void WriteOperation(ezMcpJsonWriter& ref_writer, const ezLongOpControllerManager::ProxyOpInfo& opInfo, ezStringView sName = ezStringView())
  {
    ezStringBuilder sTmp;

    // Inside an array the object has to stay anonymous, inside another object it needs a name.
    ref_writer.BeginObject(sName);

    ref_writer.AddVariableString("guid", ezConversionUtils::ToString(opInfo.m_OperationGuid, sTmp));
    ref_writer.AddVariableString("name", opInfo.m_pProxyOp->GetDisplayName());
    ref_writer.AddVariableString("type", opInfo.m_pProxyOp->GetDynamicRTTI()->GetTypeName());
    ref_writer.AddVariableString("component", ezConversionUtils::ToString(opInfo.m_ComponentGuid, sTmp));

    if (ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(opInfo.m_DocumentGuid))
    {
      ref_writer.AddVariableString("document", pDoc->GetDocumentPath());

      // The component itself carries no name, so the object it sits on is what a user would recognize.
      if (const ezDocumentObject* pComponent = pDoc->GetObjectManager()->GetObject(opInfo.m_ComponentGuid))
      {
        ref_writer.AddVariableString("componentType", pComponent->GetType()->GetTypeName());

        if (const ezDocumentObject* pOwner = pComponent->GetParent())
        {
          ref_writer.AddVariableString("object", ezMcpDocument::GetObjectName(pOwner));
        }
      }
    }

    ref_writer.AddVariableBool("running", opInfo.m_bIsRunning);
    ref_writer.AddVariableFloat("completion", opInfo.m_fCompletion);

    ref_writer.EndObject();
  }
} // namespace

void ezMcpLongOpTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  ezMcpToolDesc& list = out_tools.ExpandAndGetRef();
  list.m_sName = "longop_list";
  list.m_sDescription = "Lists the long ops currently available - the entries of the editor's 'Long Ops' panel. One is "
                        "registered automatically for every component in an open scene that offers such an operation, for "
                        "instance ezBakedProbesComponent (baking a scene). So a long op only exists while the document "
                        "holding its component is open: add the component with object_modify first if the one you want is "
                        "missing. Each entry "
                        "reports the guid that longop_execute takes, plus whether it is running and how far along it is.";
  list.m_sInputSchema = R"({"type":"object","properties":{)"
                        R"("document":{"type":"string","description":"Guid or path of an open document. Only list the long ops belonging to it."},)"
                        R"("contains":{"type":"string","description":"Only list long ops whose display name or component type contains this text, case insensitive."}}})";

  ezMcpToolDesc& run = out_tools.ExpandAndGetRef();
  run.m_sName = "longop_execute";
  run.m_sDescription = "Runs one long op and waits for it to finish, then reports whether it succeeded. This is the "
                       "equivalent of pressing its button in the property grid or the 'Long Ops' panel.\n"
                       "The work happens in the engine process, and what it writes back is applied to the document as a "
                       "single undoable step - so after this returns, object_tree shows the result and object_undo reverts "
                       "it. Nothing is saved: call document_save to keep it.\n"
                       "Unlike action_execute this call is asynchronous internally, so it does not block the editor while "
                       "waiting. It can still take minutes for a large scene, so give curl a timeout of tens of minutes. "
                       "Progress is not reported while waiting; call longop_list from another connection to see it.";
  run.m_sInputSchema = R"({"type":"object","properties":{)"
                       R"("guid":{"type":"string","description":"Guid of the long op to run, as reported by longop_list."},)"
                       R"("document":{"type":"string","description":"Guid or path of an open document. Together with 'componentType' this picks the long op instead of naming its guid."},)"
                       R"("componentType":{"type":"string","description":"Type name of the component the long op belongs to, e.g. 'ezBakedProbesComponent'. Needs 'document', and only works when that document has exactly one such component."}}})";
}

void ezMcpLongOpTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "longop_list")
  {
    ExecuteList(arguments, out_result);
  }
  else if (sToolName == "longop_execute")
  {
    ExecuteRun(arguments, out_result);
  }
}

void ezMcpLongOpTool::ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");
  const ezStringView sContains = ezMcpJson::GetString(arguments, "contains");

  ezDocument* pDocument = nullptr;

  if (!sDocument.IsEmpty())
  {
    pDocument = ezMcpDocument::Find(sDocument);

    if (pDocument == nullptr)
    {
      ezMcpDocument::SetNotOpenError(out_result, sDocument);
      return;
    }
  }

  ezLongOpControllerManager* pManager = ezLongOpControllerManager::GetSingleton();

  if (pManager == nullptr)
  {
    out_result.SetError("The long op controller is not available.");
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezUInt32 uiCount = 0;

  {
    EZ_LOCK(pManager->m_Mutex);

    writer.BeginArray("longOps");

    for (const auto& pOpInfo : pManager->GetOperations())
    {
      if (pDocument != nullptr && pOpInfo->m_DocumentGuid != pDocument->GetGuid())
        continue;

      if (!sContains.IsEmpty())
      {
        ezStringBuilder sHaystack = pOpInfo->m_pProxyOp->GetDisplayName();

        if (ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(pOpInfo->m_DocumentGuid))
        {
          if (const ezDocumentObject* pComponent = pDoc->GetObjectManager()->GetObject(pOpInfo->m_ComponentGuid))
          {
            sHaystack.Append(" ", pComponent->GetType()->GetTypeName());
          }
        }

        if (sHaystack.FindSubString_NoCase(sContains) == nullptr)
          continue;
      }

      WriteOperation(writer, *pOpInfo);
      ++uiCount;
    }

    writer.EndArray();
  }

  writer.AddVariableUInt32("count", uiCount);

  if (uiCount == 0)
  {
    writer.AddVariableString("note", "No long op matched. They only exist for components in open documents, so open the "
                                     "document and make sure it has a component that offers one.");
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

ezResult ezMcpLongOpTool::ResolveOperation(ezLongOpControllerManager& ref_manager, const ezVariantDictionary& arguments, ezUuid& out_opGuid, ezMcpToolResult& out_result)
{
  const ezStringView sGuid = ezMcpJson::GetString(arguments, "guid");
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");
  const ezStringView sComponentType = ezMcpJson::GetString(arguments, "componentType");

  if (!sGuid.IsEmpty())
  {
    if (ezConversionUtils::TryConvertStringToUuid(sGuid, out_opGuid).Failed())
    {
      out_result.SetError("'guid' is not a valid guid. Take it from longop_list.");
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  if (sDocument.IsEmpty() || sComponentType.IsEmpty())
  {
    out_result.SetError("Pass either 'guid', or both 'document' and 'componentType', to say which long op to run. "
                        "longop_list reports both.");
    return EZ_FAILURE;
  }

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return EZ_FAILURE;
  }

  ezUInt32 uiMatches = 0;

  {
    EZ_LOCK(ref_manager.m_Mutex);

    for (const auto& pOpInfo : ref_manager.GetOperations())
    {
      if (pOpInfo->m_DocumentGuid != pDocument->GetGuid())
        continue;

      const ezDocumentObject* pComponent = pDocument->GetObjectManager()->GetObject(pOpInfo->m_ComponentGuid);

      if (pComponent == nullptr || pComponent->GetType()->GetTypeName() != sComponentType)
        continue;

      out_opGuid = pOpInfo->m_OperationGuid;
      ++uiMatches;
    }
  }

  ezStringBuilder sMsg;

  if (uiMatches == 0)
  {
    sMsg.SetFormat("No long op for a component of type '{}' in that document. Use longop_list to see what is there.", sComponentType);
    out_result.SetError(sMsg);
    return EZ_FAILURE;
  }

  if (uiMatches > 1)
  {
    sMsg.SetFormat("The document has {} components of type '{}', so 'componentType' does not identify one. Pass 'guid' instead.", uiMatches, sComponentType);
    out_result.SetError(sMsg);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezMcpLongOpTool::ExecuteRun(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezLongOpControllerManager* pManager = ezLongOpControllerManager::GetSingleton();

  if (pManager == nullptr)
  {
    out_result.SetError("The long op controller is not available.");
    return;
  }

  ezUuid opGuid;
  if (ResolveOperation(*pManager, arguments, opGuid, out_result).Failed())
    return;

  // A re-entered call carries the same arguments, so it resolves to the operation that is already
  // being waited for and only has to look at its state. A different one is a second, concurrent
  // request, which this tool cannot serve because it keeps the wait state only once.
  if (m_WaitingForOp.IsValid())
  {
    if (m_WaitingForOp != opGuid)
    {
      out_result.SetError("Another long op is currently being waited for. Only one longop_execute can be in flight at a "
                          "time - watch the running one with longop_list and try again once it is done.");
      return;
    }

    bool bStillRunning = false;

    {
      EZ_LOCK(pManager->m_Mutex);

      if (auto pOpInfo = pManager->GetOperation(m_WaitingForOp))
      {
        bStillRunning = pOpInfo->m_bIsRunning;
      }
      else
      {
        // The operation disappeared, which happens when its component or document went away mid-run.
        m_WaitingForOp = ezUuid();
        out_result.SetError("The long op disappeared while it was running. Its component or document was probably closed.");
        return;
      }
    }

    if (bStillRunning)
    {
      if (ezTime::Now() - m_WaitStarted > TimeOut)
      {
        m_WaitingForOp = ezUuid();
        out_result.SetError("Timed out waiting for the long op to finish. It is still running - use longop_list to watch it.");
        return;
      }

      out_result.m_bNotFinished = true;
      return;
    }

    // Finished. The proxy op has applied its result to the document by now, since that happens in
    // Finalize(), which runs before m_bIsRunning goes back to false.
    ezMcpJsonWriter writer;
    writer.BeginObject();

    {
      EZ_LOCK(pManager->m_Mutex);

      if (auto pOpInfo = pManager->GetOperation(m_WaitingForOp))
      {
        WriteOperation(writer, *pOpInfo, "longOp");
        writer.AddVariableDouble("durationSeconds", pOpInfo->m_StartOrDuration.GetSeconds());
      }
    }

    writer.AddVariableString("note", "The long op finished. Whether it produced anything is up to the operation - check the "
                                     "editor log with log_read, and the document with object_tree. Nothing is saved yet.");
    writer.EndObject();

    m_WaitingForOp = ezUuid();
    out_result.m_sText = writer.GetResult();
    return;
  }

  {
    EZ_LOCK(pManager->m_Mutex);

    auto pOpInfo = pManager->GetOperation(opGuid);

    if (pOpInfo == nullptr)
    {
      out_result.SetError("No long op with that guid. They are per editor session, so a guid from an earlier run is stale - "
                          "call longop_list again.");
      return;
    }

    if (pOpInfo->m_bIsRunning)
    {
      out_result.SetError("That long op is already running. Wait for it to finish, watching it with longop_list.");
      return;
    }
  }

  pManager->StartOperation(opGuid);

  m_WaitingForOp = opGuid;
  m_WaitStarted = ezTime::Now();

  // Answer only once the operation is done, which needs the host to pump in between.
  out_result.m_bNotFinished = true;
}
