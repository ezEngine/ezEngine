#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/DropTool.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QDataStream>
#include <QMimeData>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpDropTool, 1, ezRTTIDefaultAllocator<ezMcpDropTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Resolves a guid or a path to an asset, see the function of the same name in AssetTool.cpp.
  ezAssetCurator::ezLockedSubAsset DropToolResolveAsset(ezStringView sPathOrGuid)
  {
    ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

    auto asset = pCurator->FindSubAsset(sPathOrGuid, false);

    if (asset.isValid())
      return asset;

    return pCurator->FindSubAsset(sPathOrGuid, true);
  }

  /// Reads a vector in either of the forms the object tools report: {"x":1,"y":2,"z":3} or [1,2,3].
  bool DropToolGetVec3(const ezVariantDictionary& arguments, ezStringView sKey, ezVec3& out_vValue)
  {
    if (const ezVariantDictionary* pDict = ezMcpJson::GetDict(arguments, sKey))
    {
      out_vValue.x = static_cast<float>(ezMcpJson::GetDouble(*pDict, "x", 0.0));
      out_vValue.y = static_cast<float>(ezMcpJson::GetDouble(*pDict, "y", 0.0));
      out_vValue.z = static_cast<float>(ezMcpJson::GetDouble(*pDict, "z", 0.0));
      return true;
    }

    if (const ezVariantArray* pArray = ezMcpJson::GetArray(arguments, sKey))
    {
      if (pArray->GetCount() != 3)
        return false;

      for (ezUInt32 i = 0; i < 3; ++i)
      {
        if (!(*pArray)[i].CanConvertTo<float>())
          return false;

        out_vValue.GetData()[i] = (*pArray)[i].ConvertTo<float>();
      }

      return true;
    }

    return false;
  }
} // namespace

void ezMcpDropTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
  desc.m_sName = "object_drop_asset";
  desc.m_sDescription =
    "Places an asset into an open scene, prefab or layer the way dragging it from the asset browser does, as one undoable step. It "
    "runs the editor's own drag & drop handler for the asset type, so it creates what a user would get: a game object with an "
    "ezMeshComponent for a mesh, an ezParticleComponent for a particle effect, an ezPrefabReferenceComponent for a prefab, and so on. "
    "Which asset types are supported depends on the loaded plugins; an unsupported one fails without changing anything. "
    "By default a new game object is created at the top level, or under 'parent'. With 'attachTo' the component is added to that "
    "existing object instead, where the asset type allows it (a prefab always becomes a new child object). With 'targetComponent' a "
    "material is assigned to that mesh component, in material slot 'slot'. "
    "'prefabInstance' creates an editable prefab instance, whose objects are part of the document and stay linked to the prefab - "
    "the same as holding Shift while dragging. "
    "Returns the objects that were added, and selects them. The document is not saved.";
  desc.m_sInputSchema = R"({"type":"object","properties":{)"
                        R"("document":{"type":"string","description":"Guid or path of an open scene, prefab or scene layer."},)"
                        R"("asset":{"type":"string","description":"Guid or path of the asset to place, as reported by 'asset_find'."},)"
                        R"("parent":{"type":"string","description":"Guid of the game object to create the new object under. Omit, or pass 'root', for a top level object."},)"
                        R"("position":{"description":"Where to place the new object, in world space: {\"x\":1,\"y\":2,\"z\":3} or [1,2,3]. Defaults to the origin."},)"
                        R"("attachTo":{"type":"string","description":"Guid of an existing game object to add the asset's component to, instead of creating a new object."},)"
                        R"("targetComponent":{"type":"string","description":"Guid of a mesh component to assign a material asset to."},)"
                        R"("slot":{"type":"integer","description":"The material slot of 'targetComponent' to assign the material to. Defaults to 0."},)"
                        R"("prefabInstance":{"type":"boolean","description":"For a prefab asset: create an editable prefab instance instead of an object with an ezPrefabReferenceComponent. Defaults to false."})"
                        R"(},"required":["document","asset"]})";
}

void ezMcpDropTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezDocument* pDocument = ezMcpDocument::FindForObjects(ezMcpJson::GetString(arguments, "document"), out_result);

  if (pDocument == nullptr)
    return;

  if (pDocument->IsReadOnly())
  {
    out_result.SetError("The document is read only and cannot be modified.");
    return;
  }

  if (ezAssetCurator::GetSingleton() == nullptr)
  {
    out_result.SetError("No project is open.");
    return;
  }

  // The handlers are found through the asset's guid, carried in the same mime data that the asset browser puts on a drag.
  const ezStringView sAsset = ezMcpJson::GetString(arguments, "asset");
  ezStringBuilder sAssetGuid;
  ezStringBuilder sAssetType;
  {
    auto asset = DropToolResolveAsset(sAsset);

    if (!asset.isValid())
    {
      ezStringBuilder s;
      s.SetFormat("No asset found for '{}'. Use 'asset_find' to search for it.", sAsset);
      out_result.SetError(s);
      return;
    }

    ezConversionUtils::ToString(asset->m_Data.m_Guid, sAssetGuid);
    sAssetType = asset->m_Data.m_sSubAssetsDocumentTypeName.GetData();
  }

  const ezStringView sParent = ezMcpJson::GetString(arguments, "parent");
  const ezStringView sAttachTo = ezMcpJson::GetString(arguments, "attachTo");
  const ezStringView sTargetComponent = ezMcpJson::GetString(arguments, "targetComponent");

  if ((!sParent.IsEmpty() ? 1 : 0) + (!sAttachTo.IsEmpty() ? 1 : 0) + (!sTargetComponent.IsEmpty() ? 1 : 0) > 1)
  {
    out_result.SetError("Pass at most one of 'parent', 'attachTo' and 'targetComponent'.");
    return;
  }

  ezStringBuilder sError;
  const ezDocumentObjectManager* pManager = pDocument->GetObjectManager();

  // The object that receives what the handler creates: the new object's parent, or the object a component is attached to.
  const ezDocumentObject* pReceiver = pManager->GetRootObject();

  ezDragDropInfo info;
  info.m_TargetDocument = pDocument->GetGuid();
  info.m_bShiftKeyDown = ezMcpJson::GetBool(arguments, "prefabInstance", false);

  // The viewport and the scene tree are the two targets that the handlers know. A viewport drop creates a new object under the
  // 'active parent', a scene tree drop onto an object (no insert index) attaches to it.
  if (!sAttachTo.IsEmpty())
  {
    pReceiver = ezMcpDocument::ResolveObject(pDocument, sAttachTo, sError);

    if (pReceiver == nullptr)
    {
      out_result.SetError(sError);
      return;
    }

    info.m_sTargetContext = "scenetree";
    info.m_TargetObject = pReceiver->GetGuid();
    info.m_iTargetObjectInsertChildIndex = -1;
  }
  else if (!sTargetComponent.IsEmpty())
  {
    const ezDocumentObject* pComponent = ezMcpDocument::ResolveObject(pDocument, sTargetComponent, sError);

    if (pComponent == nullptr)
    {
      out_result.SetError(sError);
      return;
    }

    pReceiver = pComponent;

    info.m_sTargetContext = "viewport";
    info.m_TargetComponent = pComponent->GetGuid();
    info.m_TargetObject = pComponent->GetParent() != nullptr ? pComponent->GetParent()->GetGuid() : ezUuid();
    info.m_iTargetObjectSubID = static_cast<ezInt32>(ezMcpJson::GetInt(arguments, "slot", 0));
  }
  else
  {
    if (!sParent.IsEmpty())
    {
      pReceiver = ezMcpDocument::ResolveParentObject(pDocument, sParent, sError);

      if (pReceiver == nullptr)
      {
        out_result.SetError(sError);
        return;
      }
    }

    info.m_sTargetContext = "viewport";

    if (pReceiver != pManager->GetRootObject())
      info.m_ActiveParentObject = pReceiver->GetGuid();
  }

  ezVec3 vPosition = ezVec3::MakeZero();
  const bool bHasPosition = ezMcpJson::GetDict(arguments, "position") != nullptr || ezMcpJson::GetArray(arguments, "position") != nullptr;

  if (bHasPosition && !DropToolGetVec3(arguments, "position", vPosition))
  {
    out_result.SetError("'position' must be {\"x\":..,\"y\":..,\"z\":..} or an array of three numbers.");
    return;
  }

  // The handlers are static, a drag that a user has started in a nested event loop is still running.
  if (ezDragDropHandler::IsHandlerActive())
  {
    out_result.SetError("A drag & drop operation is in progress in the editor. Try again once it is finished.");
    return;
  }

  QByteArray encodedData;
  {
    ezTempHybridArray<QString, 1> guids;
    guids.PushBack(QString::fromUtf8(sAssetGuid.GetData()));

    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    stream << guids;
  }

  QMimeData mimeData;
  mimeData.setData("application/ezEditor.AssetGuid", encodedData);
  info.m_pMimeData = &mimeData;

  ezHybridArray<ezUuid, 16> childrenBefore;
  for (const ezDocumentObject* pChild : pReceiver->GetChildren())
  {
    childrenBefore.PushBack(pChild->GetGuid());
  }

  ezCommandHistory* pHistory = pDocument->GetCommandHistory();
  const ezUInt32 uiUndoStackSizeBefore = pHistory->GetUndoStackSize();

  pHistory->StartTransaction("MCP: drop asset");

  // Called the way the real targets call them. The viewport updates the drag with every mouse move, and some handlers only apply
  // the asset during the update, e.g. a material. The scene tree only drops, and an update there would move the object that is
  // dropped onto to the drop position.
  if (!ezDragDropHandler::BeginDragDropOperation(&info))
  {
    pHistory->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("No drag & drop handler accepts a '{}' asset dropped this way onto this document. Use 'object_modify' to create an "
                "object and add the matching component instead.",
      sAssetType);
    out_result.SetError(s);
    return;
  }

  if (info.m_sTargetContext == "viewport")
    ezDragDropHandler::UpdateDragDropOperation(&info);

  ezDragDropHandler::FinishDragDrop(&info);

  ezHybridArray<const ezDocumentObject*, 16> addedObjects;
  for (const ezDocumentObject* pChild : pReceiver->GetChildren())
  {
    if (!childrenBefore.Contains(pChild->GetGuid()))
      addedObjects.PushBack(pChild);
  }

  // Positioned here rather than through the drop position: the handlers snap that to the grid, and a viewport drop without an
  // object under the mouse ignores it.
  if (bHasPosition)
  {
    ezTransform tParent = ezTransform::MakeIdentity();

    if (ezGameObjectDocument* pGameObjectDoc = ezDynamicCast<ezGameObjectDocument*>(pDocument); pGameObjectDoc != nullptr && pReceiver != pManager->GetRootObject())
    {
      tParent = pGameObjectDoc->GetGlobalTransform(pReceiver);
    }

    const ezVec3 vLocalPosition = ezTransform::MakeLocalTransform(tParent, ezTransform(vPosition)).m_vPosition;

    for (const ezDocumentObject* pObject : addedObjects)
    {
      if (pObject->GetType()->FindPropertyByName("LocalPosition") == nullptr)
        continue;

      const ezStatus res = pDocument->GetObjectAccessor()->SetValueByName(pObject, "LocalPosition", vLocalPosition);

      if (res.Failed())
      {
        pHistory->CancelTransaction();

        ezStringBuilder s;
        s.SetFormat("The asset was placed, but could not be moved to 'position': {}. Nothing was changed.", res.GetMessageString());
        out_result.SetError(s);
        return;
      }
    }
  }

  pHistory->FinishTransaction();

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("asset", sAssetGuid);
  writer.AddVariableString("assetType", sAssetType);

  writer.BeginArray("addedObjects");
  for (const ezDocumentObject* pObject : addedObjects)
  {
    writer.BeginObject();

    ezStringBuilder sGuid;
    ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
    writer.AddVariableString("guid", sGuid);

    const ezString sName = ezMcpDocument::GetObjectName(pObject);
    if (!sName.IsEmpty())
      writer.AddVariableString("name", sName);

    writer.AddVariableString("type", pObject->GetType()->GetTypeName());
    writer.EndObject();
  }
  writer.EndArray();

  // An empty transaction is discarded, which is how a handler that accepted the drop but did nothing shows, e.g. a material dropped
  // onto a component that has no material slots.
  const bool bChanged = pHistory->GetUndoStackSize() > uiUndoStackSizeBefore;
  writer.AddVariableBool("changed", bChanged);
  writer.AddVariableBool("modified", pDocument->IsModified());

  if (bChanged)
    writer.AddVariableString("note", "The document is not saved yet. Call 'document_save' to write it to disk, or 'object_undo' to take this back.");
  else
    writer.AddVariableString("note", "The drop handler accepted the asset, but did not change anything.");

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
