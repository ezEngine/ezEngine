#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/SelectionTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpSelectionTool, 1, ezRTTIDefaultAllocator<ezMcpSelectionTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMcpSelectionTool::WriteObject(ezMcpJsonWriter& ref_writer, const ezDocumentObject* pObject)
{
  ref_writer.BeginObject();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
  ref_writer.AddVariableString("guid", sGuid);

  const ezString sName = ezMcpDocument::GetObjectName(pObject);
  if (!sName.IsEmpty())
    ref_writer.AddVariableString("name", sName);

  const ezRTTI* pType = pObject->GetType();
  ref_writer.AddVariableString("type", pType != nullptr ? pType->GetTypeName() : ezStringView());

  ref_writer.EndObject();
}

void ezMcpSelectionTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "selection_get";
    desc.m_sDescription =
      "Returns which objects are selected in an open document, with their guid, name and type. Use it to find out what a user means "
      "by 'this object' or 'the selected one' before acting on it. "
      "Reports the selection in the order the objects were added, and separately the top level selection, which drops objects whose "
      "parent is also selected - that is the set an operation should usually act on, because acting on both a parent and its child "
      "applies the change twice.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document. 'document_list' reports both."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "selection_set";
    desc.m_sDescription =
      "Selects objects in an open document, which is how to show a user what is being talked about instead of describing it - the "
      "editor highlights the selection in its viewport and shows it in the property grid. Guids come from 'object_tree'. "
      "Pass an empty list to clear the selection. Objects are selected as given; guids that the document does not know are reported "
      "back rather than failing the call, so a partly stale list still selects what it can. The selection is not part of the "
      "document and is not saved, so this needs no undo and does not modify anything. "
      "A document opened without a window has a selection too, but nobody can see it.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document."},)"
                          R"("objects":{"type":"array","items":{"type":"string"},"description":"Guids of the objects to select, as reported by 'object_tree'. An empty array clears the selection."})"
                          R"(},"required":["document","objects"]})";
  }
}

void ezMcpSelectionTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "selection_get")
    ExecuteGet(arguments, out_result);
  else if (sToolName == "selection_set")
    ExecuteSet(arguments, out_result);
}

void ezMcpSelectionTool::ExecuteGet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return;
  }

  const ezSelectionManager* pSelection = pDocument->GetSelectionManager();

  if (pSelection == nullptr)
  {
    out_result.SetError("This document type has no selection.");
    return;
  }

  ezDynamicArray<ezSelectionEntry> topLevel;
  pSelection->GetTopLevelSelection(topLevel);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("document", pDocument->GetDocumentPath());
  writer.AddVariableUInt32("count", pSelection->GetSelection().GetCount());

  writer.BeginArray("selection");
  for (const ezDocumentObject* pObject : pSelection->GetSelection())
  {
    WriteObject(writer, pObject);
  }
  writer.EndArray();

  writer.BeginArray("topLevelSelection");
  for (const ezSelectionEntry& entry : topLevel)
  {
    WriteObject(writer, entry.m_pObject);
  }
  writer.EndArray();

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpSelectionTool::ExecuteSet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return;
  }

  ezSelectionManager* pSelection = pDocument->GetSelectionManager();

  if (pSelection == nullptr)
  {
    out_result.SetError("This document type has no selection.");
    return;
  }

  ezDynamicArray<ezString> objectGuids;

  // An absent 'objects' is a mistake worth reporting; an empty one is the documented way to clear the
  // selection, and the two are indistinguishable once the list is in hand.
  if (!ezMcpJson::GetStringArray(arguments, "objects", objectGuids))
  {
    out_result.SetError("Argument 'objects' is required and has to be an array of object guids. Pass an empty array to clear the "
                        "selection.");
    return;
  }

  const ezDocumentObjectManager* pManager = pDocument->GetObjectManager();

  ezDeque<const ezDocumentObject*> selection;
  ezHybridArray<ezString, 4> unknown;

  for (const ezString& sGuid : objectGuids)
  {
    // IsStringUuid() first: ConvertStringToUuid() asserts on anything that is not one, and this input
    // comes straight from a language model.
    if (!ezConversionUtils::IsStringUuid(sGuid))
    {
      unknown.PushBack(sGuid);
      continue;
    }

    const ezDocumentObject* pObject = pManager->GetObject(ezConversionUtils::ConvertStringToUuid(sGuid));

    if (pObject == nullptr)
    {
      unknown.PushBack(sGuid);
      continue;
    }

    selection.PushBack(pObject);
  }

  pSelection->SetSelection(selection);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("document", pDocument->GetDocumentPath());
  writer.AddVariableUInt32("selected", selection.GetCount());

  if (!unknown.IsEmpty())
  {
    // Named individually: with a partial match the caller needs to know which of its guids went
    // nowhere, and a count would send it back to object_tree for all of them.
    writer.BeginArray("unknownObjects");
    for (const ezString& sGuid : unknown)
    {
      writer.WriteString(sGuid);
    }
    writer.EndArray();

    writer.AddVariableString("note", "These guids are not objects of this document. Object guids are per editor session and change when "
                                     "a document is closed and reopened - list the current ones with 'object_tree'.");
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
