#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/McpTools/SceneLayerTool.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpSceneLayerTool, 1, ezRTTIDefaultAllocator<ezMcpSceneLayerTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Whether sPath ends with sSuffix at a path separator, so that 'Barrel.ezPrefab' does not match 'BigBarrel.ezPrefab'. Both cleaned.
  bool EndsWithPathElements(ezStringView sPath, ezStringView sSuffix)
  {
    if (sSuffix.IsEmpty() || !sPath.EndsWith_NoCase(sSuffix))
      return false;

    const ezUInt32 uiPathLen = sPath.GetElementCount();
    const ezUInt32 uiSuffixLen = sSuffix.GetElementCount();

    return uiPathLen == uiSuffixLen || sPath.GetStartPointer()[uiPathLen - uiSuffixLen - 1] == '/';
  }

  /// An open document by guid, absolute path or trailing part of its path, the forms the editor plugin's MCP tools accept.
  ezDocument* FindOpenDocument(ezStringView sIdentifier)
  {
    if (ezUuid guid; ezConversionUtils::TryConvertStringToUuid(sIdentifier, guid).Succeeded())
      return ezDocumentManager::GetDocumentByGuid(guid);

    ezStringBuilder sPath = sIdentifier;
    sPath.MakeCleanPath();

    if (sPath.IsEmpty())
      return nullptr;

    for (ezDocumentManager* pManager : ezDocumentManager::GetAllDocumentManagers())
    {
      for (ezDocument* pDoc : pManager->GetAllOpenDocuments())
      {
        ezStringBuilder sDocPath = pDoc->GetDocumentPath();
        sDocPath.MakeCleanPath();

        if (EndsWithPathElements(sDocPath, sPath))
          return pDoc;
      }
    }

    return nullptr;
  }

  /// The layer's file, also for layers that are not loaded. Empty if the asset curator does not know it.
  ezStringBuilder GetLayerPath(ezScene2Document* pScene, const ezUuid& layerGuid)
  {
    if (ezSceneDocument* pLayer = pScene->GetLayerDocument(layerGuid))
      return pLayer->GetDocumentPath();

    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
    if (pAsset.isValid() && pAsset->m_pAssetInfo != nullptr)
      return pAsset->m_pAssetInfo->m_Path.GetAbsolutePath();

    return {};
  }

  /// A layer named by its guid, its file name without extension, or a trailing part of its path.
  bool FindLayer(ezScene2Document* pScene, ezStringView sLayer, ezUuid& out_layerGuid)
  {
    if (ezUuid guid; ezConversionUtils::TryConvertStringToUuid(sLayer, guid).Succeeded())
    {
      out_layerGuid = guid;
      return pScene->GetLayerObject(guid) != nullptr || guid == pScene->GetGuid();
    }

    ezStringBuilder sSearch = sLayer;
    sSearch.MakeCleanPath();

    ezDynamicArray<ezUuid> layers;
    pScene->GetAllLayers(layers);

    for (const ezUuid& layerGuid : layers)
    {
      ezStringBuilder sPath = GetLayerPath(pScene, layerGuid);
      sPath.MakeCleanPath();

      if (sPath.GetFileName().IsEqual_NoCase(sSearch) || EndsWithPathElements(sPath, sSearch))
      {
        out_layerGuid = layerGuid;
        return true;
      }
    }

    return false;
  }
} // namespace

void ezMcpSceneLayerTool::WriteLayers(ezMcpJsonWriter& ref_writer, ezScene2Document* pScene)
{
  ezStringBuilder sGuid;

  ezConversionUtils::ToString(pScene->GetGuid(), sGuid);
  ref_writer.AddVariableString("scene", sGuid);

  ezConversionUtils::ToString(pScene->GetActiveLayer(), sGuid);
  ref_writer.AddVariableString("activeLayer", sGuid);

  ezDynamicArray<ezUuid> layers;
  pScene->GetAllLayers(layers);

  ref_writer.BeginArray("layers");

  for (const ezUuid& layerGuid : layers)
  {
    ref_writer.BeginObject();

    ezConversionUtils::ToString(layerGuid, sGuid);
    ref_writer.AddVariableString("guid", sGuid);

    const ezStringBuilder sPath = GetLayerPath(pScene, layerGuid);
    ref_writer.AddVariableString("name", layerGuid == pScene->GetGuid() ? ezStringView("Scene") : sPath.GetFileName());
    ref_writer.AddVariableString("path", sPath);

    if (layerGuid == pScene->GetGuid())
      ref_writer.AddVariableBool("isScene", true);

    ref_writer.AddVariableBool("active", layerGuid == pScene->GetActiveLayer());
    ref_writer.AddVariableBool("loaded", pScene->IsLayerLoaded(layerGuid));
    ref_writer.AddVariableBool("visible", pScene->IsLayerVisible(layerGuid));

    if (ezSceneDocument* pLayer = pScene->GetLayerDocument(layerGuid))
      ref_writer.AddVariableBool("modified", pLayer->IsModified());

    ref_writer.EndObject();
  }

  ref_writer.EndArray();
}

void ezMcpSceneLayerTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
  desc.m_sName = "scene_layers";
  desc.m_sDescription =
    "Lists and manages the layers of an open scene. A layer is a separate document ('.ezSceneLayer', stored next to the scene in "
    "'<Scene>_data') whose objects are part of the scene, and the scene itself is a layer as well. Only the active layer can be "
    "edited: while a layer is active, the scene's objects, undo history and selection are that layer's, so the object tools "
    "refuse a scene or layer document that is not the active one. To edit a layer, make it active, then pass either the scene "
    "or the layer as 'document' - both then reach the layer's objects. Make the scene active again to edit the scene's own "
    "objects. The user sees the active layer change in the editor. "
    "Operations: 'list' (the default) reports every layer, 'setActive', 'create' a new layer, 'delete' one, 'load' or 'unload' "
    "one (an unloaded layer is not part of the scene in the editor, but stays in the scene file), 'show' or 'hide' one in the "
    "viewport. 'create', 'delete' and unloading the active layer make the scene the active layer first. 'create' and 'delete' are "
    "undoable steps of the scene, and like any change they need 'document_save' on the scene. "
    "Each layer is saved on its own, but saving the scene saves its layers too.";
  desc.m_sInputSchema = R"({"type":"object","properties":{)"
                        R"("document":{"type":"string","description":"Guid or path of an open scene, or of one of its layers."},)"
                        R"("operation":{"type":"string","enum":["list","setActive","create","delete","load","unload","show","hide"],"description":"What to do. Defaults to 'list'."},)"
                        R"("layer":{"type":"string","description":"The layer to act on: its guid, its name, or the end of its path. 'Scene' or the scene's own guid is the scene itself (a layer named 'Scene' is reached by its guid or path). Required for every operation except 'list' and 'create'."},)"
                        R"("name":{"type":"string","description":"File name of the new layer, for 'create'. If a layer file of that name already exists next to the scene, it is added instead of a new one."})"
                        R"(},"required":["document"]})";
}

void ezMcpSceneLayerTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = FindOpenDocument(sDocument);

  if (pDocument == nullptr)
  {
    ezStringBuilder s;
    s.SetFormat("No open document matches '{}'. Pass the guid or path of an open scene - 'document_list' reports both. 'document_open' opens one.", sDocument);
    out_result.SetError(s);
    return;
  }

  ezScene2Document* pScene = ezDynamicCast<ezScene2Document*>(pDocument->GetMainDocument());

  if (pScene == nullptr)
  {
    out_result.SetError("The document is not a scene. Only scenes have layers; prefabs do not.");
    return;
  }

  const ezStringView sOperation = ezMcpJson::GetString(arguments, "operation", "list");

  // Every operation except 'show' and 'hide' can change the active layer: 'create', 'delete' and unloading the active layer switch
  // to the scene first. ezScene2Document::SetActiveLayer() asserts that no transaction is open, which happens when the user is in
  // the middle of an operation, e.g. a drag, while this call is handled.
  const bool bInTransaction = pScene->GetCommandHistory()->IsInTransaction() ||
                              (pScene->GetSceneCommandHistory() != nullptr && pScene->GetSceneCommandHistory()->IsInTransaction());

  if (sOperation != "list" && sOperation != "show" && sOperation != "hide" && bInTransaction)
  {
    ezStringBuilder s;
    s.SetFormat("The scene is in the middle of an operation in the editor, so '{}' cannot be done right now. Try again in a moment.", sOperation);
    out_result.SetError(s);
    return;
  }

  if (sOperation == "create")
  {
    const ezStringView sName = ezMcpJson::GetString(arguments, "name");

    if (sName.IsEmpty())
    {
      out_result.SetError("'create' needs a 'name' for the new layer.");
      return;
    }

    const ezString sNameString = sName;
    ezUuid layerGuid;
    ezStatus res = pScene->CreateLayer(sNameString, layerGuid);

    if (res.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Could not create the layer: {}", res.GetMessageString());
      out_result.SetError(s);
      return;
    }
  }
  else if (sOperation != "list")
  {
    const ezStringView sLayer = ezMcpJson::GetString(arguments, "layer");
    ezUuid layerGuid;

    if (sLayer.IsEqual_NoCase("Scene"))
    {
      layerGuid = pScene->GetGuid();
    }
    else if (sLayer.IsEmpty() || !FindLayer(pScene, sLayer, layerGuid))
    {
      ezStringBuilder s;
      s.SetFormat("'{}' needs a 'layer' of this scene. '{}' is not one - call 'scene_layers' with operation 'list' to see them.", sOperation, sLayer);
      out_result.SetError(s);
      return;
    }

    ezStatus res(EZ_SUCCESS);

    if (sOperation == "setActive")
    {
      res = pScene->SetActiveLayer(layerGuid);
    }
    else if (sOperation == "delete")
    {
      if (layerGuid == pScene->GetGuid())
        res = ezStatus("The scene itself cannot be deleted as a layer.");
      else
        res = pScene->DeleteLayer(layerGuid);
    }
    else if (sOperation == "load" || sOperation == "unload")
    {
      // Unloading closes the layer document without saving it.
      ezSceneDocument* pLayer = pScene->GetLayerDocument(layerGuid);
      if (sOperation == "unload" && pLayer != nullptr && pLayer != pScene && pLayer->IsModified())
        res = ezStatus("The layer has unsaved changes, which unloading would lose. Save the scene first with 'document_save'.");
      else
        res = pScene->SetLayerLoaded(layerGuid, sOperation == "load");
    }
    else if (sOperation == "show" || sOperation == "hide")
    {
      res = pScene->SetLayerVisible(layerGuid, sOperation == "show");
    }
    else
    {
      ezStringBuilder s;
      s.SetFormat("'{}' is not a valid operation. Use one of: list, setActive, create, delete, load, unload, show, hide.", sOperation);
      out_result.SetError(s);
      return;
    }

    if (res.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Could not {} the layer: {}", sOperation, res.GetMessageString());
      out_result.SetError(s);
      return;
    }
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteLayers(writer, pScene);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
