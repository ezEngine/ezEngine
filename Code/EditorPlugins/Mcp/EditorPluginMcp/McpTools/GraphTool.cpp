#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/GraphTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommentNode.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpGraphTool, 1, ezRTTIDefaultAllocator<ezMcpGraphTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// How many nodes graph_info reports in full before it says it truncated.
  constexpr ezUInt32 c_uiMaxInfoNodes = 200;

  /// How many templates one graph_node_templates call lists.
  constexpr ezUInt32 c_uiMaxTemplates = 200;

  /// Horizontal distance at which graph_add_node places a node without a position, to the right of the existing ones.
  constexpr float c_fNewNodeSpacing = 300.0f;

  /// A node creation template together with the path an agent addresses it by.
  struct GraphToolTemplate
  {
    ezVisualGraphNodeDesc m_Desc;
    ezString m_sPath;
  };

  /// The templates of the node creation menu, plus the comment entry that ezQtVisualGraphScene adds to it.
  ///
  /// Paths that occur several times get ' (2)', ' (3)', ... appended, so that every template has a path that can be passed
  /// back to graph_add_node.
  void GraphToolGetTemplates(const ezVisualGraphObjectManager* pManager, ezDynamicArray<GraphToolTemplate>& out_templates)
  {
    ezDynamicArray<ezVisualGraphNodeDesc> descs;
    pManager->GetNodeCreationTemplates(descs);

    {
      ezVisualGraphNodeDesc& commentDesc = descs.ExpandAndGetRef();
      commentDesc.m_pType = ezGetStaticRTTI<ezVisualGraphComment>();
      commentDesc.m_sTypeName = "Comment";
      commentDesc.m_sCategory = ezMakeHashedString("Misc");
    }

    ezMap<ezString, ezUInt32> pathCounts;
    ezStringBuilder sName, sPath;

    out_templates.Reserve(descs.GetCount());
    for (const ezVisualGraphNodeDesc& desc : descs)
    {
      if (desc.m_pType == nullptr)
        continue;

      ezVisualGraphObjectManager::GetNodeCreationTemplateName(desc, sName, sPath);

      ezUInt32& uiCount = pathCounts[sPath];
      ++uiCount;
      if (uiCount > 1)
        sPath.AppendFormat(" ({})", uiCount);

      GraphToolTemplate& tmpl = out_templates.ExpandAndGetRef();
      tmpl.m_Desc = desc;
      tmpl.m_sPath = sPath;
    }
  }

  /// Reads a position given as {"x":1,"y":2} or [1,2].
  bool GraphToolGetVec2(const ezVariant& value, ezVec2& out_vResult)
  {
    if (value.IsA<ezVariantDictionary>())
    {
      const ezVariantDictionary& dict = value.Get<ezVariantDictionary>();
      const ezVariant* pX = nullptr;
      const ezVariant* pY = nullptr;
      if (!dict.TryGetValue("x", pX) || !dict.TryGetValue("y", pY) || !pX->IsNumber() || !pY->IsNumber())
        return false;

      out_vResult.Set(pX->ConvertTo<float>(), pY->ConvertTo<float>());
      return true;
    }

    if (value.IsA<ezVariantArray>())
    {
      const ezVariantArray& a = value.Get<ezVariantArray>();
      if (a.GetCount() != 2 || !a[0].IsNumber() || !a[1].IsNumber())
        return false;

      out_vResult.Set(a[0].ConvertTo<float>(), a[1].ConvertTo<float>());
      return true;
    }

    return false;
  }

  ezStringBuilder GraphToolGuidToString(const ezUuid& guid)
  {
    ezStringBuilder s;
    ezConversionUtils::ToString(guid, s);
    return s;
  }

  void GraphToolWriteVec2(ezMcpJsonWriter& ref_writer, ezStringView sName, const ezVec2& v)
  {
    ref_writer.BeginObject(sName);
    ref_writer.AddVariableFloat("x", v.x);
    ref_writer.AddVariableFloat("y", v.y);
    ref_writer.EndObject();
  }

  void GraphToolAppendPinNames(ezStringBuilder& ref_sText, ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> pins)
  {
    if (pins.IsEmpty())
    {
      ref_sText.Append("none");
      return;
    }

    for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      ref_sText.Append(i > 0 ? ", '" : "'", pins[i]->GetName(), "'");
    }
  }

  /// Finds an output (bOutput) or input pin by name, with an error that lists the pins there are.
  const ezVisualGraphPin* GraphToolFindPin(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pNode, ezStringView sPin, bool bOutput, ezStringBuilder& out_sError)
  {
    const ezVisualGraphPin* pPin = bOutput ? pManager->GetOutputPinByName(pNode, sPin) : pManager->GetInputPinByName(pNode, sPin);
    if (pPin != nullptr)
      return pPin;

    const bool bIsOtherKind = (bOutput ? pManager->GetInputPinByName(pNode, sPin) : pManager->GetOutputPinByName(pNode, sPin)) != nullptr;

    out_sError.SetFormat("Node '{}' ({}) has no {} pin '{}'. ", GraphToolGuidToString(pNode->GetGuid()), pNode->GetType()->GetTypeName(), bOutput ? "output" : "input", sPin);

    if (bIsOtherKind)
    {
      out_sError.AppendFormat("It has an {} pin of that name - connections always go from an output ('source') to an input ('target'). ", bOutput ? "input" : "output");
    }

    out_sError.Append(bOutput ? "Its outputs are: " : "Its inputs are: ");
    GraphToolAppendPinNames(out_sError, bOutput ? pManager->GetOutputPins(pNode) : pManager->GetInputPins(pNode));
    out_sError.Append(".");
    return nullptr;
  }

  ezStringView GraphToolCanConnectResultToString(ezVisualGraphObjectManager::CanConnectResult result)
  {
    switch (result)
    {
      case ezVisualGraphObjectManager::CanConnectResult::Connect1to1:
        return "1to1";
      case ezVisualGraphObjectManager::CanConnectResult::Connect1toN:
        return "1toN";
      case ezVisualGraphObjectManager::CanConnectResult::ConnectNto1:
        return "Nto1";
      case ezVisualGraphObjectManager::CanConnectResult::ConnectNtoN:
        return "NtoN";
      default:
        return "never";
    }
  }
} // namespace

void ezMcpGraphTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "graph_info";
    desc.m_sDescription =
      "Describes a node graph document - visual script, material with a visual shader, render pipeline, state machine, animation "
      "graph or ProcGen graph: every node with its type, position and its input and output pins (name, data type, description, "
      "number of connections), every comment box, and every connection as source node + output pin -> target node + input pin. "
      "'otherObjects' lists top level objects that are not part of the graph, such as the asset's own settings. "
      "'graphInUse' false means the document currently ignores its graph, e.g. a material that does not use a visual shader. "
      "Node properties are read and written with 'object_properties'/'object_modify' like any other object, and 'deleteObject' "
      "removes a node together with its connections. Pins, positions and connections are only available through the graph_* tools. "
      "Pins can depend on node properties (e.g. a pin count), so read them again after changing such a property. "
      "Positions are in graph view units, y pointing down; nodes are typically 150-300 units wide.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open graph document."},)"
                          R"("nodes":{"type":"array","items":{"type":"string"},"description":"Only report these nodes, and the connections that touch them."},)"
                          R"("includePins":{"type":"boolean","description":"Report the pins of every node. Defaults to true."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "graph_node_templates";
    desc.m_sDescription =
      "Lists the kinds of nodes that can be added to a graph document, the same entries as the editor's node creation menu. Each "
      "template has a 'path' (category/name) to pass to 'graph_add_node', the node type it creates, and the property values it "
      "pre-fills - for a visual script, many templates share one generic node type and differ only in those values. "
      "Visual scripts have thousands of templates, so filter with 'contains'. At most 200 are listed per call.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open graph document."},)"
                          R"("contains":{"type":"string","description":"Only list templates whose path or type name contains this text, case insensitive."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "graph_add_node";
    desc.m_sDescription =
      "Adds a node to a graph document, as one undoable step. Name it by 'template' (a path from 'graph_node_templates', which also "
      "applies the template's pre-filled property values) or by 'type' (a node type name, accepted when only one template creates "
      "it or one of them has no pre-filled values). 'Misc/Comment' adds a comment box. Returns the new node with its pins, ready to "
      "connect with 'graph_connect'; set its other properties with 'object_modify'. "
      "Without 'position' the node is placed to the right of all existing nodes. The document is not saved.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open graph document."},)"
                          R"("template":{"type":"string","description":"Path of the template, as reported by 'graph_node_templates'."},)"
                          R"("type":{"type":"string","description":"Node type name, as an alternative to 'template'."},)"
                          R"("position":{"description":"Where to place the node, {\"x\":0,\"y\":0} or [0,0]."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "graph_move_nodes";
    desc.m_sDescription =
      "Moves nodes or comment boxes of a graph document to new positions, all in one undoable step. Positions only affect how the "
      "graph is laid out in the editor, not what it does. The document is not saved.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open graph document."},)"
                          R"("nodes":{"type":"array","items":{"type":"object","properties":{"node":{"type":"string","description":"Guid of the node."},"position":{"description":"{\"x\":0,\"y\":0} or [0,0]."}},"required":["node","position"]},"description":"The nodes to move and where to."})"
                          R"(},"required":["document","nodes"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "graph_connect";
    desc.m_sDescription =
      "Connects an output pin of one node ('source') to an input pin of another ('target'), as one undoable step. The graph type "
      "decides what may be connected - data types have to match, some graphs refuse cycles - and a refusal comes back with its reason. "
      "Many pins accept only one connection (e.g. a data input); like the editor, connecting replaces what is connected there "
      "unless 'replace' is false, in which case the call fails instead. Removed connections are reported. "
      "For a state machine, the new connection is a transition whose properties are then set with 'object_modify'. "
      "The document is not saved.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open graph document."},)"
                          R"("source":{"type":"string","description":"Guid of the node with the output pin."},)"
                          R"("sourcePin":{"type":"string","description":"Name of the output pin. Some pins have an empty name."},)"
                          R"("target":{"type":"string","description":"Guid of the node with the input pin."},)"
                          R"("targetPin":{"type":"string","description":"Name of the input pin. Some pins have an empty name."},)"
                          R"("replace":{"type":"boolean","description":"Remove connections that the new one conflicts with. Defaults to true."})"
                          R"(},"required":["document","source","sourcePin","target","targetPin"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "graph_disconnect";
    desc.m_sDescription =
      "Removes connections from a graph document, as one undoable step: one connection by its guid ('connection', as reported by "
      "'graph_info'), or every connection of a node ('node'), optionally only those of one of its pins ('pin', matching inputs and "
      "outputs of that name). The document is not saved.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open graph document."},)"
                          R"("connection":{"type":"string","description":"Guid of the connection to remove."},)"
                          R"("node":{"type":"string","description":"Guid of a node whose connections to remove, instead of 'connection'."},)"
                          R"("pin":{"type":"string","description":"With 'node': only remove the connections of this pin."})"
                          R"(},"required":["document"]})";
  }
}

void ezMcpGraphTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "graph_info")
    ExecuteInfo(arguments, out_result);
  else if (sToolName == "graph_node_templates")
    ExecuteNodeTemplates(arguments, out_result);
  else if (sToolName == "graph_add_node")
    ExecuteAddNode(arguments, out_result);
  else if (sToolName == "graph_move_nodes")
    ExecuteMoveNodes(arguments, out_result);
  else if (sToolName == "graph_connect")
    ExecuteConnect(arguments, out_result);
  else if (sToolName == "graph_disconnect")
    ExecuteDisconnect(arguments, out_result);
}

ezDocument* ezMcpGraphTool::FindGraphDocument(const ezVariantDictionary& arguments, const ezVisualGraphObjectManager*& out_pManager, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return nullptr;
  }

  out_pManager = ezMcpDocument::GetVisualGraph(pDocument);

  if (out_pManager == nullptr)
  {
    out_result.SetError("This document is not a node graph. The graph_* tools work on visual scripts, materials, render pipelines, state "
                        "machines, animation graphs and ProcGen graphs; use the object_* tools for other documents.");
    return nullptr;
  }

  return pDocument;
}

const ezDocumentObject* ezMcpGraphTool::ResolveNode(const ezDocument* pDocument, const ezVisualGraphObjectManager* pManager, ezStringView sGuid, ezStringView sArgumentName, ezMcpToolResult& out_result)
{
  if (sGuid.IsEmpty())
  {
    ezStringBuilder s;
    s.SetFormat("Argument '{}' is required and has to be a node guid, as reported by 'graph_info'.", sArgumentName);
    out_result.SetError(s);
    return nullptr;
  }

  ezStringBuilder sError;
  const ezDocumentObject* pObject = ezMcpDocument::ResolveObject(pDocument, sGuid, sError);

  if (pObject == nullptr)
  {
    out_result.SetError(sError);
    return nullptr;
  }

  if (!pManager->IsNode(pObject))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' ({}) is not a node of the graph{}.", sGuid, pObject->GetType()->GetTypeName(),
      pManager->IsComment(pObject) ? " but a comment box, which has no pins" : (pManager->IsConnection(pObject) ? " but a connection" : ""));
    out_result.SetError(s);
    return nullptr;
  }

  return pObject;
}

void ezMcpGraphTool::WritePins(ezMcpJsonWriter& ref_writer, ezStringView sName, const ezVisualGraphObjectManager* pManager, ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> pins)
{
  ezStringBuilder sDescription;

  ref_writer.BeginArray(sName);
  for (const auto& pPin : pins)
  {
    ref_writer.BeginObject();
    ref_writer.AddVariableString("name", pPin->GetName());

    if (!pPin->GetDataTypeName().IsEmpty())
      ref_writer.AddVariableString("dataType", pPin->GetDataTypeName());

    sDescription.Clear();
    pPin->GetDescription(sDescription);
    if (!sDescription.IsEmpty())
      ref_writer.AddVariableString("description", sDescription);

    ref_writer.AddVariableUInt32("connections", pManager->GetConnections(*pPin).GetCount());
    ref_writer.EndObject();
  }
  ref_writer.EndArray();
}

void ezMcpGraphTool::WriteNode(ezMcpJsonWriter& ref_writer, const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject, bool bIncludePins, ezStringView sMemberName)
{
  ref_writer.BeginObject(sMemberName);

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
  ref_writer.AddVariableString("guid", sGuid);
  ref_writer.AddVariableString("type", pObject->GetType()->GetTypeName());

  const ezString sName = ezMcpDocument::GetObjectName(pObject);
  if (!sName.IsEmpty())
    ref_writer.AddVariableString("name", sName);

  const bool bIsComment = pManager->IsComment(pObject);
  if (bIsComment)
    ref_writer.AddVariableString("kind", "comment");

  GraphToolWriteVec2(ref_writer, "position", pManager->GetNodePos(pObject));

  if (bIncludePins && !bIsComment)
  {
    WritePins(ref_writer, "inputs", pManager, pManager->GetInputPins(pObject));
    WritePins(ref_writer, "outputs", pManager, pManager->GetOutputPins(pObject));
  }

  ref_writer.EndObject();
}

void ezMcpGraphTool::WriteConnection(ezMcpJsonWriter& ref_writer, const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pConnectionObject, ezStringView sMemberName)
{
  ref_writer.BeginObject(sMemberName);

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pConnectionObject->GetGuid(), sGuid);
  ref_writer.AddVariableString("guid", sGuid);

  if (const ezVisualGraphConnection* pConnection = pManager->GetConnectionIfExists(pConnectionObject))
  {
    ezConversionUtils::ToString(pConnection->GetSourcePin().GetParent()->GetGuid(), sGuid);
    ref_writer.AddVariableString("source", sGuid);
    ref_writer.AddVariableString("sourcePin", pConnection->GetSourcePin().GetName());

    ezConversionUtils::ToString(pConnection->GetTargetPin().GetParent()->GetGuid(), sGuid);
    ref_writer.AddVariableString("target", sGuid);
    ref_writer.AddVariableString("targetPin", pConnection->GetTargetPin().GetName());
  }
  else
  {
    ref_writer.AddVariableBool("connected", false);
  }

  if (pConnectionObject->GetType() != ezGetStaticRTTI<ezDocumentObject_ConnectionBase>())
  {
    // A connection type with its own properties, such as a state machine transition.
    ref_writer.AddVariableString("type", pConnectionObject->GetType()->GetTypeName());
  }

  ref_writer.EndObject();
}

void ezMcpGraphTool::ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezVisualGraphObjectManager* pManager = nullptr;
  ezDocument* pDocument = FindGraphDocument(arguments, pManager, out_result);
  if (pDocument == nullptr)
    return;

  const bool bIncludePins = ezMcpJson::GetBool(arguments, "includePins", true);

  ezHashSet<const ezDocumentObject*> filter;
  {
    ezDynamicArray<ezString> nodeGuids;
    if (ezMcpJson::GetStringArray(arguments, "nodes", nodeGuids))
    {
      for (const ezString& sGuid : nodeGuids)
      {
        ezStringBuilder sError;
        const ezDocumentObject* pObject = ezMcpDocument::ResolveObject(pDocument, sGuid, sError);
        if (pObject == nullptr)
        {
          out_result.SetError(sError);
          return;
        }

        filter.Insert(pObject);
      }
    }
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("document", pDocument->GetDocumentPath());

  if (const ezStatus inUse = pManager->IsGraphInUse(); inUse.Failed())
  {
    writer.AddVariableBool("graphInUse", false);
    writer.AddVariableString("graphNotInUseReason", inUse.GetMessageString());
  }

  ezHybridArray<const ezDocumentObject*, 64> connections;
  ezHybridArray<const ezDocumentObject*, 8> others;
  ezUInt32 uiNodes = 0;
  bool bTruncated = false;

  writer.BeginArray("nodes");
  for (const ezDocumentObject* pObject : pManager->GetRootObject()->GetChildren())
  {
    if (pManager->IsConnection(pObject))
    {
      connections.PushBack(pObject);
      continue;
    }

    if (!pManager->IsNode(pObject) && !pManager->IsComment(pObject))
    {
      others.PushBack(pObject);
      continue;
    }

    if (!filter.IsEmpty() && !filter.Contains(pObject))
      continue;

    if (uiNodes >= c_uiMaxInfoNodes)
    {
      bTruncated = true;
      continue;
    }

    ++uiNodes;
    WriteNode(writer, pManager, pObject, bIncludePins);
  }
  writer.EndArray();

  if (bTruncated)
  {
    writer.AddVariableBool("truncated", true);
    writer.AddVariableString("note", "Not all nodes are listed. Pass 'nodes' to ask for specific ones, or 'includePins' false for a shorter overview.");
  }

  writer.BeginArray("connections");
  for (const ezDocumentObject* pConnectionObject : connections)
  {
    if (!filter.IsEmpty())
    {
      const ezVisualGraphConnection* pConnection = pManager->GetConnectionIfExists(pConnectionObject);
      if (pConnection == nullptr || (!filter.Contains(pConnection->GetSourcePin().GetParent()) && !filter.Contains(pConnection->GetTargetPin().GetParent())))
        continue;
    }

    WriteConnection(writer, pManager, pConnectionObject);
  }
  writer.EndArray();

  if (!others.IsEmpty() && filter.IsEmpty())
  {
    writer.BeginArray("otherObjects");
    for (const ezDocumentObject* pObject : others)
    {
      writer.BeginObject();
      ezStringBuilder sGuid;
      ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
      writer.AddVariableString("guid", sGuid);
      writer.AddVariableString("type", pObject->GetType()->GetTypeName());
      writer.EndObject();
    }
    writer.EndArray();
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpGraphTool::ExecuteNodeTemplates(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezVisualGraphObjectManager* pManager = nullptr;
  ezDocument* pDocument = FindGraphDocument(arguments, pManager, out_result);
  if (pDocument == nullptr)
    return;

  const ezStringView sContains = ezMcpJson::GetString(arguments, "contains");

  ezDynamicArray<GraphToolTemplate> templates;
  GraphToolGetTemplates(pManager, templates);

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableUInt32("totalTemplates", templates.GetCount());

  ezUInt32 uiMatching = 0;

  writer.BeginArray("templates");
  for (const GraphToolTemplate& tmpl : templates)
  {
    if (!sContains.IsEmpty() && tmpl.m_sPath.FindSubString_NoCase(sContains) == nullptr && ezStringView(tmpl.m_Desc.m_pType->GetTypeName()).FindSubString_NoCase(sContains) == nullptr)
      continue;

    ++uiMatching;
    if (uiMatching > c_uiMaxTemplates)
      continue;

    writer.BeginObject();
    writer.AddVariableString("path", tmpl.m_sPath);
    writer.AddVariableString("type", tmpl.m_Desc.m_pType->GetTypeName());

    if (!tmpl.m_Desc.m_PropertyValues.IsEmpty())
    {
      writer.BeginObject("properties");
      for (const ezVisualGraphNodeProperty& prop : tmpl.m_Desc.m_PropertyValues)
      {
        writer.AddVariableVariant(prop.m_sPropertyName.GetView(), prop.m_Value);
      }
      writer.EndObject();
    }

    writer.EndObject();
  }
  writer.EndArray();

  writer.AddVariableUInt32("matching", uiMatching);
  if (uiMatching > c_uiMaxTemplates)
  {
    writer.AddVariableBool("truncated", true);
    writer.AddVariableString("note", "Only the first 200 matching templates are listed. Narrow the search with 'contains'.");
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpGraphTool::ExecuteAddNode(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezVisualGraphObjectManager* pManager = nullptr;
  ezDocument* pDocument = FindGraphDocument(arguments, pManager, out_result);
  if (pDocument == nullptr)
    return;

  if (pDocument->IsReadOnly())
  {
    out_result.SetError("The document is read only and cannot be modified.");
    return;
  }

  if (const ezStatus inUse = pManager->IsGraphInUse(); inUse.Failed())
  {
    ezStringBuilder s;
    s.SetFormat("{} Change that first with 'object_modify', or nothing added to the graph has an effect.", inUse.GetMessageString());
    out_result.SetError(s);
    return;
  }

  const ezStringView sTemplate = ezMcpJson::GetString(arguments, "template");
  const ezStringView sType = ezMcpJson::GetString(arguments, "type");

  if (sTemplate.IsEmpty() == sType.IsEmpty())
  {
    out_result.SetError("Pass either 'template' (a path from 'graph_node_templates') or 'type' (a node type name), not both or neither.");
    return;
  }

  ezDynamicArray<GraphToolTemplate> templates;
  GraphToolGetTemplates(pManager, templates);

  const GraphToolTemplate* pTemplate = nullptr;

  if (!sTemplate.IsEmpty())
  {
    for (const GraphToolTemplate& tmpl : templates)
    {
      if (tmpl.m_sPath.IsEqual_NoCase(sTemplate))
      {
        pTemplate = &tmpl;
        break;
      }
    }

    if (pTemplate == nullptr)
    {
      ezStringBuilder s;
      s.SetFormat("There is no node template '{}'. Look up the exact path with 'graph_node_templates', using 'contains' with a part of the name.", sTemplate);
      out_result.SetError(s);
      return;
    }
  }
  else
  {
    ezHybridArray<const GraphToolTemplate*, 8> candidates;
    for (const GraphToolTemplate& tmpl : templates)
    {
      if (ezStringView(tmpl.m_Desc.m_pType->GetTypeName()).IsEqual_NoCase(sType))
        candidates.PushBack(&tmpl);
    }

    if (candidates.IsEmpty())
    {
      ezStringBuilder s;
      s.SetFormat("No node template of this graph creates type '{}'. 'graph_node_templates' lists the types that can be added here.", sType);
      out_result.SetError(s);
      return;
    }

    pTemplate = candidates[0];

    if (candidates.GetCount() > 1)
    {
      pTemplate = nullptr;
      for (const GraphToolTemplate* pCandidate : candidates)
      {
        if (pCandidate->m_Desc.m_PropertyValues.IsEmpty())
        {
          pTemplate = pCandidate;
          break;
        }
      }

      if (pTemplate == nullptr)
      {
        ezStringBuilder s;
        s.SetFormat("{} templates create type '{}' with different pre-filled values, so it does not say which node is meant. Pass "
                    "'template' instead; 'graph_node_templates' with 'contains' lists them.",
          candidates.GetCount(), sType);
        out_result.SetError(s);
        return;
      }
    }
  }

  ezVec2 vPos = ezVec2::MakeZero();

  if (const ezVariant* pPosition = nullptr; arguments.TryGetValue("position", pPosition))
  {
    if (!GraphToolGetVec2(*pPosition, vPos))
    {
      out_result.SetError("'position' has to be {\"x\":0,\"y\":0} or [0,0].");
      return;
    }
  }
  else
  {
    bool bAny = false;
    ezVec2 vMax = ezVec2::MakeZero();
    float fMinY = 0.0f;

    for (const ezDocumentObject* pObject : pManager->GetRootObject()->GetChildren())
    {
      if (!pManager->IsNode(pObject))
        continue;

      const ezVec2 vNodePos = pManager->GetNodePos(pObject);
      vMax.x = bAny ? ezMath::Max(vMax.x, vNodePos.x) : vNodePos.x;
      fMinY = bAny ? ezMath::Min(fMinY, vNodePos.y) : vNodePos.y;
      bAny = true;
    }

    if (bAny)
      vPos.Set(vMax.x + c_fNewNodeSpacing, fMinY);
  }

  ezCommandHistory* pHistory = pDocument->GetCommandHistory();

  ezStringBuilder sTransaction;
  sTransaction.SetFormat("MCP: add node '{}'", pTemplate->m_sPath);
  pHistory->StartTransaction(sTransaction);

  ezAddObjectCommand addCmd;
  addCmd.m_pType = pTemplate->m_Desc.m_pType;
  addCmd.m_NewObjectGuid = ezUuid::MakeUuid();
  addCmd.m_Index = -1;

  ezStatus res = pHistory->AddCommand(addCmd);

  if (res.Succeeded())
  {
    ezMoveNodeCommand moveCmd;
    moveCmd.m_Object = addCmd.m_NewObjectGuid;
    moveCmd.m_NewPos = vPos;
    res = pHistory->AddCommand(moveCmd);
  }

  for (const ezVisualGraphNodeProperty& prop : pTemplate->m_Desc.m_PropertyValues)
  {
    if (res.Failed())
      break;

    ezSetObjectPropertyCommand setCmd;
    setCmd.m_Object = addCmd.m_NewObjectGuid;
    setCmd.m_sProperty = prop.m_sPropertyName.GetString();
    setCmd.m_NewValue = prop.m_Value;
    res = pHistory->AddCommand(setCmd);
  }

  if (res.Failed())
  {
    pHistory->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("Could not add the node: {}", res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pHistory->FinishTransaction();

  const ezDocumentObject* pNewObject = pManager->GetObject(addCmd.m_NewObjectGuid);

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("template", pTemplate->m_sPath);
  WriteNode(writer, pManager, pNewObject, true, "node");
  writer.AddVariableBool("modified", pDocument->IsModified());
  writer.AddVariableString("note", "The document is not saved yet. Call 'document_save' to write it to disk, or 'object_undo' to take this back.");
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpGraphTool::ExecuteMoveNodes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezVisualGraphObjectManager* pManager = nullptr;
  ezDocument* pDocument = FindGraphDocument(arguments, pManager, out_result);
  if (pDocument == nullptr)
    return;

  if (pDocument->IsReadOnly())
  {
    out_result.SetError("The document is read only and cannot be modified.");
    return;
  }

  const ezVariantArray* pNodes = ezMcpJson::GetArray(arguments, "nodes");
  if (pNodes == nullptr || pNodes->IsEmpty())
  {
    out_result.SetError("'nodes' is required: an array of {\"node\":\"<guid>\",\"position\":{\"x\":0,\"y\":0}}.");
    return;
  }

  struct Move
  {
    const ezDocumentObject* m_pObject = nullptr;
    ezVec2 m_vPos;
  };

  ezHybridArray<Move, 16> moves;

  for (const ezVariant& entry : *pNodes)
  {
    if (!entry.IsA<ezVariantDictionary>())
    {
      out_result.SetError("Every element of 'nodes' has to be an object {\"node\":\"<guid>\",\"position\":{\"x\":0,\"y\":0}}.");
      return;
    }

    const ezVariantDictionary& dict = entry.Get<ezVariantDictionary>();
    const ezStringView sGuid = ezMcpJson::GetString(dict, "node");

    ezStringBuilder sError;
    const ezDocumentObject* pObject = ezMcpDocument::ResolveObject(pDocument, sGuid, sError);
    if (pObject == nullptr)
    {
      out_result.SetError(sGuid.IsEmpty() ? ezStringView("Every element of 'nodes' needs a 'node' guid.") : sError.GetView());
      return;
    }

    Move& move = moves.ExpandAndGetRef();
    move.m_pObject = pObject;

    const ezVariant* pPosition = nullptr;
    if (!dict.TryGetValue("position", pPosition) || !GraphToolGetVec2(*pPosition, move.m_vPos))
    {
      ezStringBuilder s;
      s.SetFormat("The entry for '{}' needs a 'position', {\"x\":0,\"y\":0} or [0,0].", sGuid);
      out_result.SetError(s);
      return;
    }

    const ezStatus canMove = pManager->CanMoveNode(pObject, move.m_vPos);
    if (canMove.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("'{}' cannot be moved: {}", sGuid, canMove.GetMessageString());
      out_result.SetError(s);
      return;
    }
  }

  ezCommandHistory* pHistory = pDocument->GetCommandHistory();
  pHistory->StartTransaction("MCP: move nodes");

  for (const Move& move : moves)
  {
    ezMoveNodeCommand moveCmd;
    moveCmd.m_Object = move.m_pObject->GetGuid();
    moveCmd.m_NewPos = move.m_vPos;

    const ezStatus res = pHistory->AddCommand(moveCmd);
    if (res.Failed())
    {
      pHistory->CancelTransaction();

      ezStringBuilder s;
      s.SetFormat("Could not move '{}': {}", GraphToolGuidToString(move.m_pObject->GetGuid()), res.GetMessageString());
      out_result.SetError(s);
      return;
    }
  }

  pHistory->FinishTransaction();

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableUInt32("moved", moves.GetCount());
  writer.AddVariableBool("modified", pDocument->IsModified());
  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpGraphTool::ExecuteConnect(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezVisualGraphObjectManager* pManager = nullptr;
  ezDocument* pDocument = FindGraphDocument(arguments, pManager, out_result);
  if (pDocument == nullptr)
    return;

  if (pDocument->IsReadOnly())
  {
    out_result.SetError("The document is read only and cannot be modified.");
    return;
  }

  if (const ezStatus inUse = pManager->IsGraphInUse(); inUse.Failed())
  {
    ezStringBuilder s;
    s.SetFormat("{} Change that first with 'object_modify', or nothing added to the graph has an effect.", inUse.GetMessageString());
    out_result.SetError(s);
    return;
  }

  const ezDocumentObject* pSource = ResolveNode(pDocument, pManager, ezMcpJson::GetString(arguments, "source"), "source", out_result);
  if (pSource == nullptr)
    return;

  const ezDocumentObject* pTarget = ResolveNode(pDocument, pManager, ezMcpJson::GetString(arguments, "target"), "target", out_result);
  if (pTarget == nullptr)
    return;

  // Pin names can be empty, e.g. the execution input of some visual script nodes, so only a missing argument is an error.
  const ezVariant* pSourcePinArg = nullptr;
  const ezVariant* pTargetPinArg = nullptr;
  if (!arguments.TryGetValue("sourcePin", pSourcePinArg) || !arguments.TryGetValue("targetPin", pTargetPinArg))
  {
    out_result.SetError("'sourcePin' and 'targetPin' are required. 'graph_info' lists the pins of each node.");
    return;
  }

  ezStringBuilder sError;
  const ezVisualGraphPin* pSourcePin = GraphToolFindPin(pManager, pSource, ezMcpJson::GetString(arguments, "sourcePin"), true, sError);
  if (pSourcePin == nullptr)
  {
    out_result.SetError(sError);
    return;
  }

  const ezVisualGraphPin* pTargetPin = GraphToolFindPin(pManager, pTarget, ezMcpJson::GetString(arguments, "targetPin"), false, sError);
  if (pTargetPin == nullptr)
  {
    out_result.SetError(sError);
    return;
  }

  ezVisualGraphObjectManager::CanConnectResult connect = ezVisualGraphObjectManager::CanConnectResult::ConnectNever;
  const ezStatus canConnect = pManager->CanConnect(pManager->GetConnectionType(), *pSourcePin, *pTargetPin, connect);

  // Only ConnectNever is a refusal. Some graphs report a failed status together with another result to say that the connection
  // is possible but replaces existing ones, which is how ezQtVisualGraphScene::ConnectPinsAction() treats it as well.
  if (connect == ezVisualGraphObjectManager::CanConnectResult::ConnectNever)
  {
    ezStringBuilder s;
    s.SetFormat("These pins cannot be connected: {}", canConnect.GetMessageString().IsEmpty() ? ezStringView("the graph does not allow it.") : canConnect.GetMessageString().GetView());
    out_result.SetError(s);
    return;
  }

  // Which existing connections the new one replaces, as in ezQtVisualGraphScene::ConnectPinsAction().
  ezHybridArray<const ezDocumentObject*, 4> toRemove;
  {
    auto Collect = [&](const ezVisualGraphPin& pin)
    {
      for (const ezVisualGraphConnection* pConnection : pManager->GetConnections(pin))
      {
        if (!toRemove.Contains(pConnection->GetParent()))
          toRemove.PushBack(pConnection->GetParent());
      }
    };

    if (connect == ezVisualGraphObjectManager::CanConnectResult::Connect1to1 || connect == ezVisualGraphObjectManager::CanConnectResult::Connect1toN)
      Collect(*pSourcePin);

    if (connect == ezVisualGraphObjectManager::CanConnectResult::Connect1to1 || connect == ezVisualGraphObjectManager::CanConnectResult::ConnectNto1)
      Collect(*pTargetPin);
  }

  if (!toRemove.IsEmpty() && !ezMcpJson::GetBool(arguments, "replace", true))
  {
    ezStringBuilder s;
    s.SetFormat("Connecting these pins would replace {} existing connection(s), because the graph allows '{}' connections here (output-to-input "
                "count). Call again with 'replace' true, or remove them with 'graph_disconnect' first.",
      toRemove.GetCount(), GraphToolCanConnectResultToString(connect));
    out_result.SetError(s);
    return;
  }

  ezHybridArray<ezUuid, 4> removedGuids;
  for (const ezDocumentObject* pConnectionObject : toRemove)
  {
    removedGuids.PushBack(pConnectionObject->GetGuid());
  }

  ezCommandHistory* pHistory = pDocument->GetCommandHistory();
  pHistory->StartTransaction("MCP: connect pins");

  ezStatus res(EZ_SUCCESS);

  for (const ezUuid& guid : removedGuids)
  {
    res = ezNodeCommands::DisconnectAndRemoveCommand(pHistory, guid);
    if (res.Failed())
      break;
  }

  if (res.Succeeded())
  {
    res = ezNodeCommands::AddAndConnectCommand(pHistory, pManager->GetConnectionType(), *pSourcePin, *pTargetPin);
  }

  if (res.Failed())
  {
    pHistory->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("Could not connect the pins: {}", res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pHistory->FinishTransaction();

  const ezDocumentObject* pNewConnection = nullptr;
  for (const ezVisualGraphConnection* pConnection : pManager->GetConnections(*pSourcePin))
  {
    if (&pConnection->GetTargetPin() == pTargetPin)
    {
      pNewConnection = pConnection->GetParent();
      break;
    }
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  if (pNewConnection != nullptr)
  {
    WriteConnection(writer, pManager, pNewConnection, "connection");
  }

  if (!removedGuids.IsEmpty())
  {
    ezStringBuilder sGuid;
    writer.BeginArray("removedConnections");
    for (const ezUuid& guid : removedGuids)
    {
      ezConversionUtils::ToString(guid, sGuid);
      writer.WriteString(sGuid);
    }
    writer.EndArray();
  }

  writer.AddVariableBool("modified", pDocument->IsModified());
  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpGraphTool::ExecuteDisconnect(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezVisualGraphObjectManager* pManager = nullptr;
  ezDocument* pDocument = FindGraphDocument(arguments, pManager, out_result);
  if (pDocument == nullptr)
    return;

  if (pDocument->IsReadOnly())
  {
    out_result.SetError("The document is read only and cannot be modified.");
    return;
  }

  const ezStringView sConnection = ezMcpJson::GetString(arguments, "connection");
  const ezStringView sNode = ezMcpJson::GetString(arguments, "node");

  if (sConnection.IsEmpty() == sNode.IsEmpty())
  {
    out_result.SetError("Pass either 'connection' (a connection guid) or 'node' (a node guid, optionally with 'pin').");
    return;
  }

  ezHybridArray<const ezDocumentObject*, 8> toRemove;

  if (!sConnection.IsEmpty())
  {
    ezStringBuilder sError;
    const ezDocumentObject* pObject = ezMcpDocument::ResolveObject(pDocument, sConnection, sError);
    if (pObject == nullptr)
    {
      out_result.SetError(sError);
      return;
    }

    if (!pManager->IsConnection(pObject) || pManager->GetConnectionIfExists(pObject) == nullptr)
    {
      out_result.SetError("That object is not a connection. 'graph_info' lists the connections under 'connections'.");
      return;
    }

    toRemove.PushBack(pObject);
  }
  else
  {
    const ezDocumentObject* pNode = ResolveNode(pDocument, pManager, sNode, "node", out_result);
    if (pNode == nullptr)
      return;

    const ezVariant* pPinArg = nullptr;
    const bool bFilterPin = arguments.TryGetValue("pin", pPinArg);
    const ezStringView sPin = ezMcpJson::GetString(arguments, "pin");
    bool bPinFound = false;

    auto Collect = [&](ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> pins)
    {
      for (const auto& pPin : pins)
      {
        if (bFilterPin && pPin->GetName() != sPin)
          continue;

        bPinFound = true;
        for (const ezVisualGraphConnection* pConnection : pManager->GetConnections(*pPin))
        {
          if (!toRemove.Contains(pConnection->GetParent()))
            toRemove.PushBack(pConnection->GetParent());
        }
      }
    };

    Collect(pManager->GetInputPins(pNode));
    Collect(pManager->GetOutputPins(pNode));

    if (bFilterPin && !bPinFound)
    {
      ezStringBuilder s;
      s.SetFormat("Node '{}' has no pin '{}'. Its inputs are: ", sNode, sPin);
      GraphToolAppendPinNames(s, pManager->GetInputPins(pNode));
      s.Append(". Its outputs are: ");
      GraphToolAppendPinNames(s, pManager->GetOutputPins(pNode));
      s.Append(".");
      out_result.SetError(s);
      return;
    }
  }

  for (const ezDocumentObject* pConnectionObject : toRemove)
  {
    const ezStatus canDisconnect = pManager->CanDisconnect(pConnectionObject);
    if (canDisconnect.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Connection '{}' cannot be removed: {}", GraphToolGuidToString(pConnectionObject->GetGuid()), canDisconnect.GetMessageString());
      out_result.SetError(s);
      return;
    }
  }

  ezHybridArray<ezUuid, 8> removedGuids;
  for (const ezDocumentObject* pConnectionObject : toRemove)
  {
    removedGuids.PushBack(pConnectionObject->GetGuid());
  }

  if (!removedGuids.IsEmpty())
  {
    ezCommandHistory* pHistory = pDocument->GetCommandHistory();
    pHistory->StartTransaction("MCP: disconnect pins");

    for (const ezUuid& guid : removedGuids)
    {
      const ezStatus res = ezNodeCommands::DisconnectAndRemoveCommand(pHistory, guid);
      if (res.Failed())
      {
        pHistory->CancelTransaction();

        ezStringBuilder s;
        s.SetFormat("Could not remove connection '{}': {}", GraphToolGuidToString(guid), res.GetMessageString());
        out_result.SetError(s);
        return;
      }
    }

    pHistory->FinishTransaction();
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezStringBuilder sGuid;
  writer.BeginArray("removedConnections");
  for (const ezUuid& guid : removedGuids)
  {
    ezConversionUtils::ToString(guid, sGuid);
    writer.WriteString(sGuid);
  }
  writer.EndArray();

  writer.AddVariableBool("modified", pDocument->IsModified());
  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}
