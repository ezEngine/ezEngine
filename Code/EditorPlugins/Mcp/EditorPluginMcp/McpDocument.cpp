#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <Mcp/McpTool.h>

#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

ezDocument* ezMcpDocument::Find(ezStringView sIdentifier)
{
  if (sIdentifier.IsEmpty())
    return nullptr;

  // TryConvertStringToUuid() rather than ConvertStringToUuid(), which asserts on anything that is not
  // already a uuid and so cannot be used to find out whether the input is one. The input here is a
  // path just as often as a guid.
  if (ezUuid guid; ezConversionUtils::TryConvertStringToUuid(sIdentifier, guid).Succeeded())
  {
    // A guid that is not open is not a path either, so there is nothing else to try.
    return ezDocumentManager::GetDocumentByGuid(guid);
  }

  ezStringBuilder sPath = sIdentifier;
  sPath.MakeCleanPath();

  if (sPath.IsEmpty())
    return nullptr;

  // Matched against the open documents' own paths rather than resolved through the data directories
  // first. Only documents that are already open can match, so their paths are the complete set of right
  // answers - and going through ezQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute() instead
  // would dereference ezSubAsset::m_pAssetInfo, which is null for an asset the curator knows but has no
  // file information for. That crashed the editor when a path was passed for a document that had just
  // been closed.
  for (ezDocumentManager* pManager : ezDocumentManager::GetAllDocumentManagers())
  {
    if (ezPathUtils::IsAbsolutePath(sPath))
    {
      if (ezDocument* pDoc = pManager->GetDocumentByPath(sPath))
        return pDoc;

      continue;
    }

    // A relative path matches when it is a trailing path component sequence of the absolute one, so
    // both "Testing Chambers/Prefabs/Barrel.ezPrefab" and "Prefabs/Barrel.ezPrefab" find it.
    for (ezDocument* pDoc : pManager->GetAllOpenDocuments())
    {
      if (pDoc == nullptr)
        continue;

      ezStringBuilder sDocPath = pDoc->GetDocumentPath();
      sDocPath.MakeCleanPath();

      if (sDocPath.EndsWith_NoCase(sPath))
      {
        // Guards against 'Barrel.ezPrefab' matching 'BigBarrel.ezPrefab': what precedes the match has
        // to be a path separator, not more filename.
        const ezUInt32 uiDocLen = sDocPath.GetElementCount();
        const ezUInt32 uiPathLen = sPath.GetElementCount();

        if (uiDocLen == uiPathLen || sDocPath.GetData()[uiDocLen - uiPathLen - 1] == '/')
          return pDoc;
      }
    }
  }

  return nullptr;
}

void ezMcpDocument::SetNotOpenError(ezMcpToolResult& out_result, ezStringView sIdentifier)
{
  // Names both accepted forms and the tool that lists what is open, because an agent that guessed a
  // path wrong otherwise has no way to find out what it should have passed.
  ezStringBuilder s;
  s.SetFormat("No open document matches '{}'. Pass the guid or the path of a document that is currently open - "
              "'document_list' returns both for every open document. To open a document that is not open yet, use 'document_open'.",
    sIdentifier);

  out_result.SetError(s);
}

ezDocument* ezMcpDocument::FindForObjects(ezStringView sIdentifier, ezMcpToolResult& out_result)
{
  ezDocument* pDocument = Find(sIdentifier);

  if (pDocument == nullptr)
  {
    SetNotOpenError(out_result, sIdentifier);
    return nullptr;
  }

  const ezDocument* pActive = pDocument->GetMainDocument()->GetActiveSubDocument();

  if (pActive == nullptr || pActive == pDocument)
    return pDocument;

  ezStringBuilder sActiveGuid;
  ezConversionUtils::ToString(pActive->GetGuid(), sActiveGuid);

  ezStringBuilder s;
  if (pDocument->IsMainDocument())
  {
    s.SetFormat("'{}' currently has its layer '{}' active, and while it is, the objects, undo history and selection of the scene are "
                "those of that layer. Pass the layer ('{}') as 'document' to work on the layer, or make the scene itself the active layer "
                "with 'scene_layers' to work on the scene's own objects. Note that the user sees the active layer change.",
      pDocument->GetDocumentPath(), pActive->GetDocumentPath(), sActiveGuid);
  }
  else
  {
    ezStringBuilder sMainGuid;
    ezConversionUtils::ToString(pDocument->GetMainDocument()->GetGuid(), sMainGuid);

    s.SetFormat("'{}' is a layer of '{}', and only the active layer can be read or edited. The active one is '{}' ('{}'). Make this layer the "
                "active one with 'scene_layers' (document '{}'), which the user sees as well.",
      pDocument->GetDocumentPath(), pDocument->GetMainDocument()->GetDocumentPath(), pActive->GetDocumentPath(), sActiveGuid, sMainGuid);
  }

  out_result.SetError(s);
  return nullptr;
}

const ezDocumentObject* ezMcpDocument::ResolveParentObject(const ezDocument* pDocument, ezStringView sObjectGuid, ezStringBuilder& out_sError)
{
  if (sObjectGuid == s_sRootObject)
    return pDocument->GetObjectManager()->GetRootObject();

  return ResolveObject(pDocument, sObjectGuid, out_sError);
}

const ezDocumentObject* ezMcpDocument::ResolveObject(const ezDocument* pDocument, ezStringView sObjectGuid, ezStringBuilder& out_sError)
{
  const ezDocumentObjectManager* pManager = pDocument->GetObjectManager();

  if (!sObjectGuid.IsEmpty())
  {
    if (!ezConversionUtils::IsStringUuid(sObjectGuid))
    {
      out_sError.SetFormat("'{}' is not a valid object guid. Omit 'object' to address the document's top level object.", sObjectGuid);
      return nullptr;
    }

    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sObjectGuid);
    const ezDocumentObject* pObject = pManager->GetObject(guid);

    // The document's root is a real object with a stable guid - ezUuid::MakeStableUuidFromString("DocumentRoot"),
    // the same in every document - so it is reachable by name even though no tool ever reports it. It is
    // an implementation detail with no parent and no type a caller can act on, and handing it out lets
    // the object tools try to delete, move or duplicate it, which dereferences its absent parent.
    if (pObject != nullptr && pObject == pManager->GetRootObject())
    {
      out_sError = "That guid is the document's internal root object, which cannot be read or modified. Omit 'object' to address the "
                   "document's top level object, or use 'object_tree' to list the objects that can be acted on.";
      return nullptr;
    }

    if (pObject == nullptr)
    {
      out_sError.SetFormat("The document has no object with guid '{}'. Object guids come from a previous 'object_tree' or "
                           "'object_properties' call and are not stable across editor sessions.",
        sObjectGuid);
    }

    return pObject;
  }

  const ezDocumentObject* pRoot = pManager->GetRootObject();

  if (pRoot == nullptr || pRoot->GetChildren().IsEmpty())
  {
    out_sError = "The document has no top level object.";
    return nullptr;
  }

  if (pRoot->GetChildren().GetCount() > 1)
  {
    out_sError.SetFormat("The document has {} top level objects, so there is no single one to default to. Pass 'object' with the guid "
                         "of the one to act on - 'object_tree' lists them. To create a new top level object, pass 'root' as 'object'.",
      pRoot->GetChildren().GetCount());
    return nullptr;
  }

  return pRoot->GetChildren()[0];
}

ezString ezMcpDocument::GetObjectName(const ezDocumentObject* pObject)
{
  const ezRTTI* pType = pObject->GetType();

  if (pType == nullptr || pType->FindPropertyByName("Name") == nullptr)
    return {};

  const ezVariant name = pObject->GetTypeAccessor().GetValue("Name");

  if (!name.IsValid() || !name.CanConvertTo<ezString>())
    return {};

  return name.ConvertTo<ezString>();
}

const ezVisualGraphObjectManager* ezMcpDocument::GetVisualGraph(const ezDocument* pDocument)
{
  const ezObjectAccessorBase* pAccessor = pDocument->GetObjectAccessor();

  if (pAccessor == nullptr || !pAccessor->GetDynamicRTTI()->IsDerivedFrom<ezVisualGraphCommandAccessor>())
    return nullptr;

  return static_cast<const ezVisualGraphObjectManager*>(pDocument->GetObjectManager());
}
