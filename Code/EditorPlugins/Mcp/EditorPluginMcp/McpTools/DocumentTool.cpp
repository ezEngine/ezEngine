#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/DocumentTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Document/DocumentUtils.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <QFile>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpDocumentTool, 1, ezRTTIDefaultAllocator<ezMcpDocumentTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Turns whatever the caller passed into an absolute path.
  ///
  /// An agent has three plausible spellings of the same document: the absolute path, the data
  /// directory parent relative path that asset_find reports ("Testing Chambers/Prefabs/Barrel.ezPrefab")
  /// and the data directory relative path. All three are accepted, because rejecting a path the agent
  /// read out of another tool's output is the kind of failure it cannot diagnose.
  ///
  /// Returns an empty string if nothing could be made of the input.
  ezStringBuilder DocumentToolResolvePath(ezStringView sPath)
  {
    ezStringBuilder sResult = sPath;
    sResult.MakeCleanPath();

    if (sResult.IsEmpty() || ezPathUtils::IsAbsolutePath(sResult))
      return sResult;

    // Deliberately not passing a guid to MakeParentDataDirectoryRelativePathAbsolute(): it resolves
    // one through ezSubAsset::m_pAssetInfo without checking it for null, which crashes the editor for
    // an asset the curator knows but holds no file information for. Callers that want to name a
    // document by guid go through ezMcpDocument::Find() instead.
    if (ezConversionUtils::IsStringUuid(sResult))
      return sResult;

    // The parent relative form first: it is what the asset tools report, so it is the form an agent
    // is most likely to be carrying around.
    ezStringBuilder sTemp = sResult;
    if (ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sTemp, false))
      return sTemp;

    sTemp = sResult;
    if (ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTemp))
      return sTemp;

    return sResult;
  }

} // namespace

void ezMcpDocumentTool::WriteDocumentIdentity(ezMcpJsonWriter& ref_writer, const ezDocument& document)
{
  ezStringBuilder sGuid;
  ezConversionUtils::ToString(document.GetGuid(), sGuid);

  ref_writer.AddVariableString("guid", sGuid);
  ref_writer.AddVariableString("path", document.GetDocumentPath());
  ref_writer.AddVariableString("type", document.GetDocumentTypeName());
  ref_writer.AddVariableBool("modified", document.IsModified());

  if (document.IsReadOnly())
    ref_writer.AddVariableBool("readOnly", true);

  // A sub-document is edited through its main document, so a caller that got one here needs to know
  // that acting on it is not the same as acting on a normal document.
  if (document.IsSubDocument())
    ref_writer.AddVariableBool("subDocument", true);
}

void ezMcpDocumentTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_types";
    desc.m_sDescription =
      "Lists the document types the editor can open and create, with the file extension each one uses. Call this before "
      "'document_create': the type name is what 'document_create' takes, and the extension determines it when opening a path. "
      "'canCreate' false means the type only ever results from importing or generating a file, so 'document_create' will refuse it. "
      "This is a different list from 'asset_types' - it includes non-asset documents and is about what can be edited rather than "
      "what the asset database tracks.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"Only list types whose name or extension contains this, case insensitive."},)"
                          R"("canCreate":{"type":"boolean","description":"If true, only list types that 'document_create' accepts."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_list";
    desc.m_sDescription =
      "Lists the documents that are currently open in the editor, with guid, absolute path, type and whether they have unsaved "
      "changes. The guid is the stable identifier every other document tool accepts. Note that a document can be open without a "
      "visible window - the editor opens documents internally to resolve references - so this is not the same as what the user sees "
      "in their tabs; 'hasWindow' tells the two apart.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("type":{"type":"string","description":"Only list documents of this document type, e.g. 'Scene'. See 'document_types'."},)"
                          R"("modifiedOnly":{"type":"boolean","description":"If true, only list documents with unsaved changes."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_open";
    desc.m_sDescription =
      "Opens an existing document and returns its guid. Does nothing but return the document if it is already open, so this is also "
      "the way to get the guid of a document by path. The type is determined by the file extension. Opening a scene can take a while, "
      "as it starts the engine process - see 'app_ping' if the call times out.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("path":{"type":"string","description":"Absolute path, or the path relative to the data directory parent as reported by 'asset_find'."},)"
                          R"("focus":{"type":"boolean","description":"Open a visible window and bring it to the front. Defaults to true. Pass false to open the document only internally, without disturbing what the user is looking at."})"
                          R"(},"required":["path"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_create";
    desc.m_sDescription =
      "Creates a new document on disk and opens it. The new document is saved immediately, so the file exists when this returns. "
      "Fails if the file already exists - delete it first or pick another path. By default the editor's template for the type is "
      "copied, which gives the document the same starting state a user would get; pass 'empty' to get a document with no content "
      "beyond what the type requires.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("path":{"type":"string","description":"Where to create it. Absolute, or relative to the data directory parent. Must be inside the project. The file extension must match the document type - if it is omitted the extension of the type is appended."},)"
                          R"("type":{"type":"string","description":"The document type to create, e.g. 'Prefab'. See 'document_types'. Optional if the path already carries the type's extension."},)"
                          R"("empty":{"type":"boolean","description":"If true, do not copy the type's default template. Defaults to false, i.e. use the template, which is what the editor's own 'new document' does."},)"
                          R"("focus":{"type":"boolean","description":"Open a visible window for it and bring it to the front. Defaults to true."})"
                          R"(},"required":["path"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_save";
    desc.m_sDescription =
      "Writes an open document to disk. Does nothing if it has no unsaved changes, unless 'force' is set. Saving is what makes a "
      "change visible to the asset system, so a document that was modified through other tools has to be saved before transforming "
      "it or before anything referencing it is transformed.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document. Omit together with 'all'."},)"
                          R"("all":{"type":"boolean","description":"If true, save every open document with unsaved changes instead of one named document."},)"
                          R"("force":{"type":"boolean","description":"Write the file even if the document is not modified. Defaults to false."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_close";
    desc.m_sDescription =
      "Closes an open document. This DISCARDS unsaved changes without asking, so it refuses to close a modified document unless "
      "'discardChanges' is set - save it first with 'document_save' if the changes matter. Closing a document the user was working "
      "in makes its window disappear, so prefer leaving documents open unless there is a reason to close them.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document."},)"
                          R"("discardChanges":{"type":"boolean","description":"Required to close a document with unsaved changes. Those changes are lost permanently. Defaults to false."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_focus";
    desc.m_sDescription =
      "Brings an open document's window to the front, so the user is looking at it. Use this to show the user what a change refers "
      "to instead of describing where to find it. Has no effect on a document that was opened without a window.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "document_delete";
    desc.m_sDescription =
      "Deletes a document file, optionally repointing everything that references it at a replacement first. Deleting a referenced "
      "asset without a replacement leaves broken references behind, so this reports how many other assets use it and refuses to "
      "proceed unless either 'replaceWith' or 'force' is given. With 'replaceWith', every referencing document is opened, rewritten "
      "and saved, and the original is only deleted if all of that succeeded. The file goes to the recycle bin. Prefer this over "
      "deleting an asset file directly, which does none of the reference fixing.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("path":{"type":"string","description":"Guid or path of the document to delete. It does not have to be open."},)"
                          R"("replaceWith":{"type":"string","description":"Guid or path of the asset that should take its place in every document that references it. Should be of the same type."},)"
                          R"("force":{"type":"boolean","description":"Delete even though other assets reference it and no replacement was given, leaving those references broken. Defaults to false."})"
                          R"(},"required":["path"]})";
  }
}

void ezMcpDocumentTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "document_types")
    ExecuteListTypes(arguments, out_result);
  else if (sToolName == "document_list")
    ExecuteList(arguments, out_result);
  else if (sToolName == "document_open")
    ExecuteOpen(arguments, out_result);
  else if (sToolName == "document_create")
    ExecuteCreate(arguments, out_result);
  else if (sToolName == "document_save")
    ExecuteSave(arguments, out_result);
  else if (sToolName == "document_close")
    ExecuteClose(arguments, out_result);
  else if (sToolName == "document_focus")
    ExecuteFocus(arguments, out_result);
  else if (sToolName == "document_delete")
    ExecuteDelete(arguments, out_result);
}

void ezMcpDocumentTool::ExecuteListTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sNameFilter = ezMcpJson::GetString(arguments, "name");
  const bool bCanCreateOnly = ezMcpJson::GetBool(arguments, "canCreate", false);

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.BeginArray("types");

  ezUInt32 uiCount = 0;

  for (auto it : ezDocumentManager::GetAllDocumentDescriptors())
  {
    const ezDocumentTypeDescriptor* pDesc = it.Value();

    if (pDesc == nullptr)
      continue;

    if (bCanCreateOnly && !pDesc->m_bCanCreate)
      continue;

    if (!sNameFilter.IsEmpty())
    {
      const bool bMatches = pDesc->m_sDocumentTypeName.FindSubString_NoCase(sNameFilter) != nullptr ||
                            pDesc->m_sFileExtension.FindSubString_NoCase(sNameFilter) != nullptr;
      if (!bMatches)
        continue;
    }

    ++uiCount;

    writer.BeginObject();
    writer.AddVariableString("name", pDesc->m_sDocumentTypeName);
    writer.AddVariableString("extension", pDesc->m_sFileExtension);
    writer.AddVariableBool("canCreate", pDesc->m_bCanCreate);

    // Same trap as in AssetTool: the lookup returns the key unchanged when there is no entry, so an
    // untranslated type would otherwise be reported as having a display name identical to its name.
    const ezStringView sDisplayName = ezTranslate(pDesc->m_sDocumentTypeName.GetData());
    if (!sDisplayName.IsEmpty() && sDisplayName != pDesc->m_sDocumentTypeName)
      writer.AddVariableString("displayName", sDisplayName);

    const ezStringView sHelpUrl = ezTranslateHelpURL(pDesc->m_sDocumentTypeName.GetData());
    if (!sHelpUrl.IsEmpty() && sHelpUrl != pDesc->m_sDocumentTypeName)
      writer.AddVariableString("helpUrl", sHelpUrl);

    writer.EndObject();
  }

  writer.EndArray();
  writer.AddVariableUInt32("count", uiCount);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpDocumentTool::ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sTypeFilter = ezMcpJson::GetString(arguments, "type");
  const bool bModifiedOnly = ezMcpJson::GetBool(arguments, "modifiedOnly", false);

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.BeginArray("documents");

  ezUInt32 uiCount = 0;

  for (const ezDocumentManager* pManager : ezDocumentManager::GetAllDocumentManagers())
  {
    for (const ezDocument* pDoc : pManager->GetAllOpenDocuments())
    {
      if (pDoc == nullptr)
        continue;

      if (bModifiedOnly && !pDoc->IsModified())
        continue;

      if (!sTypeFilter.IsEmpty() && !pDoc->GetDocumentTypeName().IsEqual_NoCase(sTypeFilter))
        continue;

      ++uiCount;

      writer.BeginObject();
      WriteDocumentIdentity(writer, *pDoc);

      // Distinguishes a document the user is looking at from one the editor opened behind the scenes.
      writer.AddVariableBool("hasWindow", pDoc->HasWindowBeenRequested());

      writer.EndObject();
    }
  }

  writer.EndArray();
  writer.AddVariableUInt32("count", uiCount);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpDocumentTool::ExecuteOpen(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sPath = ezMcpJson::GetString(arguments, "path");

  if (sPath.IsEmpty())
  {
    out_result.SetError("No 'path' argument given.");
    return;
  }

  const bool bFocus = ezMcpJson::GetBool(arguments, "focus", true);

  const ezStringBuilder sAbsPath = DocumentToolResolvePath(sPath);

  // A path that resolved against no data directory is still relative, and everything below expects an
  // absolute one: ezOSFile::ExistsFile() asserts on a relative path rather than returning false, which
  // takes the editor down with it.
  if (!ezPathUtils::IsAbsolutePath(sAbsPath))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' could not be resolved to a file in this project. Pass an absolute path, or the path as 'asset_find' reports it "
                "(data directory name first, e.g. 'Testing Chambers/Effects/SmallExplosion.ezParticleEffectAsset').",
      sPath);
    out_result.SetError(s);
    return;
  }

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(sAbsPath, false, pTypeDesc).Failed() || pTypeDesc == nullptr)
  {
    ezStringBuilder s;
    s.SetFormat("The file extension of '{}' is not registered with any document type. 'document_types' lists the extensions that are.", sAbsPath);
    out_result.SetError(s);
    return;
  }

  ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath);
  const bool bWasAlreadyOpen = pDocument != nullptr;

  if (!bWasAlreadyOpen)
  {
    if (!ezOSFile::ExistsFile(sAbsPath))
    {
      ezStringBuilder s;
      s.SetFormat("There is no file at '{}'. Use 'document_create' to create a new document, or 'asset_find' to locate an existing one.", sAbsPath);
      out_result.SetError(s);
      return;
    }

    // Deliberately not going through ezQtEditorApp::OpenDocument(): that reports every failure with a
    // modal message box, which never returns when there is no human to close it.
    ezStatus res = pTypeDesc->m_pManager->CanOpenDocument(sAbsPath);

    if (res.Succeeded())
    {
      ezBitflags<ezDocumentFlags> flags = ezDocumentFlags::AddToRecentFilesList;
      if (bFocus)
        flags |= ezDocumentFlags::RequestWindow;

      res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, sAbsPath, pDocument, flags);
    }

    if (res.Failed() || pDocument == nullptr)
    {
      ezStringBuilder s;
      s.SetFormat("Failed to open '{}': {}", sAbsPath, res.GetMessageString());
      out_result.SetError(s);
      return;
    }
  }

  if (bFocus)
  {
    // Covers the already-open case too, where no window was requested the first time round.
    pTypeDesc->m_pManager->EnsureWindowRequested(pDocument);
    pDocument->EnsureVisible();
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteDocumentIdentity(writer, *pDocument);
  writer.AddVariableBool("wasAlreadyOpen", bWasAlreadyOpen);

  // Loading errors are reported here rather than as a failure: the document did open, and an agent
  // that asked for it should be told it is incomplete rather than that nothing happened.
  if (pDocument->GetUnknownObjectTypeInstances() > 0)
    writer.AddVariableUInt32("unknownObjectTypeInstances", pDocument->GetUnknownObjectTypeInstances());

  if (!pDocument->GetLoadingErrors().IsEmpty())
  {
    writer.BeginArray("loadingErrors");
    for (const ezString& sError : pDocument->GetLoadingErrors())
    {
      writer.WriteString(sError);
    }
    writer.EndArray();
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpDocumentTool::ExecuteCreate(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sPath = ezMcpJson::GetString(arguments, "path");

  if (sPath.IsEmpty())
  {
    out_result.SetError("No 'path' argument given.");
    return;
  }

  const ezStringView sType = ezMcpJson::GetString(arguments, "type");
  const bool bEmpty = ezMcpJson::GetBool(arguments, "empty", false);
  const bool bFocus = ezMcpJson::GetBool(arguments, "focus", true);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (!sType.IsEmpty())
  {
    pTypeDesc = ezDocumentManager::GetDescriptorForDocumentType(sType);

    if (pTypeDesc == nullptr)
    {
      ezStringBuilder s;
      s.SetFormat("'{}' is not a known document type. 'document_types' lists the valid names.", sType);
      out_result.SetError(s);
      return;
    }

    // Checked here, while the type is still the thing the caller named. Later on the failure would
    // come out of the path checks instead and blame the file extension, which is not the problem.
    if (!pTypeDesc->m_bCanCreate)
    {
      ezStringBuilder s;
      s.SetFormat("Documents of type '{}' cannot be created this way - they are produced by importing or generating a file. "
                  "Call 'document_types' with 'canCreate' to see which types can be created.",
        pTypeDesc->m_sDocumentTypeName);
      out_result.SetError(s);
      return;
    }
  }

  ezStringBuilder sAbsPath = DocumentToolResolvePath(sPath);

  // A named type whose extension is missing from the path is a typo waiting to happen, so append it
  // rather than failing - the type was stated unambiguously. But only when the path carries no
  // document extension at all: appending to a path that already names a *different* type would turn a
  // contradiction ('Thing.ezScene' as a Prefab) into a 'Thing.ezScene.ezPrefab' nobody asked for,
  // instead of the error below.
  if (pTypeDesc != nullptr)
  {
    const ezStringView sExtension = ezPathUtils::GetFileExtension(sAbsPath);

    const ezDocumentTypeDescriptor* pExtensionTypeDesc = nullptr;
    const bool bExtensionIsKnown =
      !sExtension.IsEmpty() && ezDocumentManager::FindDocumentTypeFromPath(sAbsPath, true, pExtensionTypeDesc).Succeeded();

    if (!bExtensionIsKnown && !sExtension.IsEqual_NoCase(pTypeDesc->m_sFileExtension))
    {
      sAbsPath.Append(".", pTypeDesc->m_sFileExtension);
    }
  }

  if (!ezPathUtils::IsAbsolutePath(sAbsPath))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' could not be resolved to a location inside the project. Pass an absolute path, or one relative to the data "
                "directory parent such as 'Testing Chambers/Prefabs/Thing.ezPrefab'. 'project_info' reports the data directories.",
      sPath);
    out_result.SetError(s);
    return;
  }

  if (ezOSFile::ExistsFile(sAbsPath))
  {
    ezStringBuilder s;
    s.SetFormat("A file already exists at '{}'. Creating would overwrite it, which this tool does not do. Pick another path, or use "
                "'document_open' if this is the document you meant.",
      sAbsPath);
    out_result.SetError(s);
    return;
  }

  {
    // Checks the extension is registered and that no open document already claims the path.
    const ezDocumentTypeDescriptor* pPathTypeDesc = nullptr;
    const ezStatus res = ezDocumentUtils::IsValidSaveLocationForDocument(sAbsPath, &pPathTypeDesc);

    if (res.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Cannot create '{}': {}", sAbsPath, res.GetMessageString());
      out_result.SetError(s);
      return;
    }

    if (pTypeDesc == nullptr)
      pTypeDesc = pPathTypeDesc;
    else if (pPathTypeDesc != nullptr && pPathTypeDesc != pTypeDesc)
    {
      // Two different answers to 'what type is this' - guessing which one the caller meant would
      // silently create the wrong kind of document.
      ezStringBuilder s;
      s.SetFormat("The requested type '{}' does not match the file extension '{}', which belongs to type '{}'. Use the extension "
                  "'{}' for a '{}' document.",
        pTypeDesc->m_sDocumentTypeName, ezPathUtils::GetFileExtension(sAbsPath), pPathTypeDesc->m_sDocumentTypeName,
        pTypeDesc->m_sFileExtension, pTypeDesc->m_sDocumentTypeName);
      out_result.SetError(s);
      return;
    }
  }

  if (pTypeDesc == nullptr || pTypeDesc->m_pManager == nullptr)
  {
    out_result.SetError("Could not determine the document type to create. Pass 'type' explicitly - 'document_types' lists the valid names.");
    return;
  }

  // ezDocumentManager::CreateDocument() asserts on a type that cannot be created, and an assert takes
  // the whole editor down. The same check runs above for an explicitly named type; this one catches
  // the case where the type came from the path's extension.
  if (!pTypeDesc->m_bCanCreate)
  {
    ezStringBuilder s;
    s.SetFormat("Documents of type '{}' cannot be created this way - they are produced by importing or generating a file. "
                "Call 'document_types' with 'canCreate' to see which types can be created.",
      pTypeDesc->m_sDocumentTypeName);
    out_result.SetError(s);
    return;
  }

  if (!ezToolsProject::IsProjectOpen())
  {
    out_result.SetError("No project is open, so no document can be created.");
    return;
  }

  if (!ezToolsProject::GetSingleton()->IsDocumentInAllowedRoot(sAbsPath))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' is outside the open project. Documents have to be created inside one of the project's data directories - "
                "'project_info' lists them.",
      sAbsPath);
    out_result.SetError(s);
    return;
  }

  ezBitflags<ezDocumentFlags> flags = ezDocumentFlags::AddToRecentFilesList;

  if (bFocus)
    flags |= ezDocumentFlags::RequestWindow;

  // Without this the manager clones Editor/DocumentTemplates/Default.<ext> when one exists, which is
  // what the editor's own 'new document' does and usually the more useful starting point.
  if (bEmpty)
    flags |= ezDocumentFlags::EmptyDocument;

  ezDocument* pDocument = nullptr;
  const ezStatus res = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, sAbsPath, pDocument, flags);

  if (res.Failed() || pDocument == nullptr)
  {
    ezStringBuilder s;
    s.SetFormat("Failed to create '{}': {}", sAbsPath, res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  // Tells the asset system about the new file. Without it the document exists but the asset database
  // does not know about it until something else triggers a file system scan.
  ezFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);

  if (bFocus)
    pDocument->EnsureVisible();

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteDocumentIdentity(writer, *pDocument);
  writer.AddVariableBool("fromTemplate", !bEmpty);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpDocumentTool::ExecuteSave(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sIdentifier = ezMcpJson::GetString(arguments, "document");
  const bool bAll = ezMcpJson::GetBool(arguments, "all", false);
  const bool bForce = ezMcpJson::GetBool(arguments, "force", false);

  if (!bAll && sIdentifier.IsEmpty())
  {
    out_result.SetError("Pass either 'document' to save one document, or 'all' to save every modified document.");
    return;
  }

  ezHybridArray<ezDocument*, 16> toSave;

  if (bAll)
  {
    for (ezDocumentManager* pManager : ezDocumentManager::GetAllDocumentManagers())
    {
      for (ezDocument* pDoc : pManager->GetAllOpenDocuments())
      {
        if (pDoc != nullptr && (bForce || pDoc->IsModified()))
          toSave.PushBack(pDoc);
      }
    }
  }
  else
  {
    ezDocument* pDocument = ezMcpDocument::Find(sIdentifier);

    if (pDocument == nullptr)
    {
      ezMcpDocument::SetNotOpenError(out_result, sIdentifier);
      return;
    }

    toSave.PushBack(pDocument);
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.BeginArray("saved");

  ezUInt32 uiSaved = 0;
  ezUInt32 uiFailed = 0;
  ezUInt32 uiUnchanged = 0;

  for (ezDocument* pDoc : toSave)
  {
    if (!bForce && !pDoc->IsModified())
    {
      ++uiUnchanged;
      continue;
    }

    const ezStatus res = pDoc->SaveDocument(bForce);

    writer.BeginObject();
    WriteDocumentIdentity(writer, *pDoc);

    if (res.Succeeded())
    {
      ++uiSaved;
      writer.AddVariableBool("success", true);
    }
    else
    {
      ++uiFailed;
      writer.AddVariableBool("success", false);
      writer.AddVariableString("error", res.GetMessageString());
    }

    writer.EndObject();
  }

  writer.EndArray();
  writer.AddVariableUInt32("savedCount", uiSaved);

  if (uiFailed > 0)
    writer.AddVariableUInt32("failedCount", uiFailed);

  // Says 'there was nothing to do' rather than leaving the caller to infer it from an empty array.
  if (uiUnchanged > 0)
    writer.AddVariableUInt32("alreadyUpToDate", uiUnchanged);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
  out_result.m_bIsError = uiFailed > 0;
}

void ezMcpDocumentTool::ExecuteClose(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sIdentifier = ezMcpJson::GetString(arguments, "document");

  if (sIdentifier.IsEmpty())
  {
    out_result.SetError("No 'document' argument given.");
    return;
  }

  ezDocument* pDocument = ezMcpDocument::Find(sIdentifier);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sIdentifier);
    return;
  }

  // ezDocumentManager::CloseDocument() throws unsaved changes away without a word, and the usual
  // 'ask the user' path is a modal dialog this tool must not open. So the decision is the caller's,
  // and the default is the safe one.
  if (pDocument->IsModified() && !ezMcpJson::GetBool(arguments, "discardChanges", false))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' has unsaved changes. Save it with 'document_save' first, or pass 'discardChanges' to close it and lose them.",
      pDocument->GetDocumentPath());
    out_result.SetError(s);
    return;
  }

  // Everything needed for the result has to be read before the document is deleted.
  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteDocumentIdentity(writer, *pDocument);
  writer.AddVariableBool("closed", true);
  writer.EndObject();

  pDocument->GetDocumentManager()->CloseDocument(pDocument);

  out_result.m_sText = writer.GetResult();
}

void ezMcpDocumentTool::ExecuteFocus(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sIdentifier = ezMcpJson::GetString(arguments, "document");

  if (sIdentifier.IsEmpty())
  {
    out_result.SetError("No 'document' argument given.");
    return;
  }

  ezDocument* pDocument = ezMcpDocument::Find(sIdentifier);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sIdentifier);
    return;
  }

  // A document opened with focus:false has no window yet, so asking for one first makes this work
  // rather than silently doing nothing.
  const bool bHadWindow = pDocument->HasWindowBeenRequested();

  if (!bHadWindow)
    pDocument->GetDocumentManager()->EnsureWindowRequested(pDocument);

  pDocument->EnsureVisible();

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteDocumentIdentity(writer, *pDocument);
  writer.AddVariableBool("windowCreated", !bHadWindow);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpDocumentTool::ExecuteDelete(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sIdentifier = ezMcpJson::GetString(arguments, "path");

  if (sIdentifier.IsEmpty())
  {
    out_result.SetError("No 'path' argument given.");
    return;
  }

  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("The asset curator is not available, so references cannot be checked and nothing is deleted.");
    return;
  }

  ezUuid assetGuid;
  ezStringBuilder sAbsPath;
  ezString sAssetTypeName;

  {
    auto asset = pCurator->FindSubAsset(sIdentifier, false);

    if (!asset.isValid())
      asset = pCurator->FindSubAsset(sIdentifier, true);

    if (!asset.isValid())
    {
      ezStringBuilder s;
      s.SetFormat("No asset matches '{}'. Pass the guid or the path of an existing asset - 'asset_find' locates them.", sIdentifier);
      out_result.SetError(s);
      return;
    }

    if (asset->m_pAssetInfo == nullptr)
    {
      out_result.SetError("The asset is known but has no file information, so it cannot be deleted safely.");
      return;
    }

    assetGuid = asset->m_Data.m_Guid;
    sAbsPath = asset->m_pAssetInfo->m_Path.GetAbsolutePath();
    sAssetTypeName = asset->m_Data.m_sSubAssetsDocumentTypeName.GetString();
  }

  const ezStringView sReplacement = ezMcpJson::GetString(arguments, "replaceWith");
  const bool bForce = ezMcpJson::GetBool(arguments, "force", false);

  ezSet<ezUuid> uses;
  pCurator->FindAllUses(assetGuid, uses, true);

  // Refusing here rather than deleting is the whole point of routing a delete through this tool: the
  // caller may well not know the asset is used, and broken references surface much later as a
  // transform failure somewhere unrelated.
  if (!uses.IsEmpty() && sReplacement.IsEmpty() && !bForce)
  {
    ezStringBuilder s;
    s.SetFormat("'{}' is referenced by {} other asset(s). Pass 'replaceWith' to repoint them at a different asset of type '{}' "
                "before deleting, or 'force' to delete anyway and leave those references broken. 'asset_uses' lists what uses it.",
      sAbsPath, uses.GetCount(), sAssetTypeName);
    out_result.SetError(s);
    return;
  }

  ezUuid replacementGuid;

  if (!sReplacement.IsEmpty())
  {
    auto replacement = pCurator->FindSubAsset(sReplacement, false);

    if (!replacement.isValid())
      replacement = pCurator->FindSubAsset(sReplacement, true);

    if (!replacement.isValid())
    {
      ezStringBuilder s;
      s.SetFormat("No asset matches the replacement '{}'.", sReplacement);
      out_result.SetError(s);
      return;
    }

    replacementGuid = replacement->m_Data.m_Guid;

    if (replacementGuid == assetGuid)
    {
      out_result.SetError("The replacement is the same asset as the one being deleted.");
      return;
    }

    // A replacement of a different type would be written into the referencing documents and only fail
    // later, when something tries to load it as the type the property expects.
    if (!replacement->m_Data.m_sSubAssetsDocumentTypeName.GetString().IsEqual_NoCase(sAssetTypeName))
    {
      ezStringBuilder s;
      s.SetFormat("The replacement is of type '{}' but '{}' is of type '{}'. Replacing across types would write references that the "
                  "using documents cannot resolve.",
        replacement->m_Data.m_sSubAssetsDocumentTypeName, sAbsPath, sAssetTypeName);
      out_result.SetError(s);
      return;
    }
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(assetGuid, sGuid);
  writer.AddVariableString("guid", sGuid);
  writer.AddVariableString("path", sAbsPath);
  writer.AddVariableUInt32("referencedBy", uses.GetCount());

  if (replacementGuid.IsValid())
  {
    ezStringBuilder sOldReference, sNewReference;
    ezConversionUtils::ToString(assetGuid, sOldReference);
    ezConversionUtils::ToString(replacementGuid, sNewReference);

    const ezAssetCurator::ReplaceAssetResult result = pCurator->ReplaceAssetReferenceInUses(assetGuid, sOldReference, sNewReference);

    writer.AddVariableString("replacedWith", sNewReference);
    writer.AddVariableUInt32("documentsModified", result.m_uiDocumentsModified);
    writer.AddVariableUInt32("propertiesReplaced", result.m_uiPropertiesReplaced);

    if (!result.m_Errors.IsEmpty())
    {
      writer.BeginArray("errors");
      for (const ezString& sError : result.m_Errors)
      {
        writer.WriteString(sError);
      }
      writer.EndArray();
    }

    // Deleting now would strand exactly the references that failed to move, so the asset stays and
    // the caller gets to decide what to do about it.
    if (result.m_uiDocumentsFailed > 0)
    {
      writer.AddVariableUInt32("documentsFailed", result.m_uiDocumentsFailed);
      writer.AddVariableBool("deleted", false);
      writer.EndObject();

      out_result.m_sText = writer.GetResult();
      out_result.m_bIsError = true;
      return;
    }
  }

  // The document has to go before the file does, otherwise the editor holds an open document whose
  // file no longer exists and will happily save it back out again.
  ezDocumentManager::EnsureDocumentIsClosedInAllManagers(sAbsPath);

  // Qt's, because there is no ez equivalent - the asset browser deletes the same way. The recycle bin
  // rather than an outright delete matters here: this tool can be called by mistake.
  if (!QFile::moveToTrash(ezMakeQString(sAbsPath)))
  {
    writer.AddVariableBool("deleted", false);
    writer.AddVariableString("error", "The file could not be moved to the recycle bin. It may be open in another program or write protected.");
    writer.EndObject();

    out_result.m_sText = writer.GetResult();
    out_result.m_bIsError = true;
    return;
  }

  ezFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);

  writer.AddVariableBool("deleted", true);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
