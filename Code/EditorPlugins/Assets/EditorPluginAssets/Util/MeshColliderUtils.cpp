#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

namespace
{
  constexpr ezStringView s_sMeshIncludeTags = "MeshIncludeTags"_ezsv;
  constexpr ezStringView s_sMeshExcludeTags = "MeshExcludeTags"_ezsv;

  constexpr ezStringView s_ColliderSubMeshProperties[] = {s_sMeshIncludeTags, s_sMeshExcludeTags};

  constexpr ezStringView s_ColliderImportProperties[] = {
    "MeshFile"_ezsv,
    s_sMeshIncludeTags,
    s_sMeshExcludeTags,
    "ImportTransform"_ezsv,
    "RightDir"_ezsv,
    "UpDir"_ezsv,
    "FlipForwardDir"_ezsv,
    "PositionOffset"_ezsv,
    "UniformScaling"_ezsv,
  };

  constexpr ezStringView s_ColliderSimplificationProperties[] = {
    "SimplifyMesh"_ezsv,
    "MeshSimplification"_ezsv,
    "MaxSimplificationError"_ezsv,
    "NormalWeight"_ezsv,
    "AggressiveSimplification"_ezsv,
  };

  /// An unwritten property and one set to an empty string mean the same thing here.
  ezString GetTagValue(const ezVariantDictionary& properties, ezStringView sProperty)
  {
    ezVariant value;
    if (!properties.TryGetValue(sProperty, value) || !value.IsA<ezString>())
      return {};

    return value.Get<ezString>();
  }

  /// Equivalent of ezSimpleAssetDocument::GetPropertyObject(), which can't be called without knowing
  /// the concrete asset type.
  const ezDocumentObject* GetColliderTopLevelObject(const ezDocument* pDoc)
  {
    const ezDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject();
    if (pRoot == nullptr || pRoot->GetChildren().GetCount() != 1)
      return nullptr;

    return pRoot->GetChildren()[0];
  }
} // namespace

bool ezMeshColliderUtils::IsMeshAsset(const ezUuid& assetGuid)
{
  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (!pSubAsset.isValid() || pSubAsset->m_pAssetInfo == nullptr || pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor == nullptr)
    return false;

  const ezStringView sType = pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;
  return sType == s_sMeshDocType || sType == s_sAnimatedMeshDocType;
}

ezArrayPtr<const ezStringView> ezMeshColliderUtils::GetImportPropertyNames()
{
  return ezMakeArrayPtr(s_ColliderImportProperties);
}

ezArrayPtr<const ezStringView> ezMeshColliderUtils::GetSimplificationPropertyNames()
{
  return ezMakeArrayPtr(s_ColliderSimplificationProperties);
}

ezArrayPtr<const ezStringView> ezMeshColliderUtils::GetSubMeshPropertyNames()
{
  return ezMakeArrayPtr(s_ColliderSubMeshProperties);
}

ezStringView ezMeshColliderUtils::GetDocumentType(ezEnum<ezCollisionMeshKind> kind)
{
  return (kind == ezCollisionMeshKind::ConvexHull) ? "Jolt_Colmesh_Convex"_ezsv : "Jolt_Colmesh_Triangle"_ezsv;
}

ezStringView ezMeshColliderUtils::GetExtension(ezEnum<ezCollisionMeshKind> kind)
{
  return (kind == ezCollisionMeshKind::ConvexHull) ? "ezJoltConvexCollisionMeshAsset"_ezsv : "ezJoltCollisionMeshAsset"_ezsv;
}

ezUuid ezMeshColliderUtils::FindExisting(ezEnum<ezCollisionMeshKind> kind, ezStringView sMeshFile, const ezVariantDictionary& meshImportProperties, ezStringView sMeshAssetPath)
{
  if (sMeshFile.IsEmpty())
    return {};

  const ezStringView sDocType = GetDocumentType(kind);

  struct Candidate
  {
    ezUuid m_Guid;
    ezString m_sPath;
  };

  ezHybridArray<Candidate, 8> candidates;

  auto pAssets = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
  for (auto it : *pAssets)
  {
    const ezSubAsset& subAsset = it.Value();
    if (!subAsset.m_bMainAsset || subAsset.m_pAssetInfo == nullptr || subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor == nullptr)
      continue;

    if (subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName != sDocType)
      continue;

    const ezAssetDocumentInfo* pInfo = subAsset.m_pAssetInfo->m_Info.Borrow();
    if (pInfo != nullptr && pInfo->m_TransformDependencies.Contains(sMeshFile))
    {
      candidates.PushBack({subAsset.m_Data.m_Guid, subAsset.m_pAssetInfo->m_Path.GetAbsolutePath()});
    }
  }

  if (candidates.IsEmpty())
    return {};

  // Which sub-mesh a candidate selects is only stored inside its document, so every candidate has to
  // be read, even a single one: it may belong to a different mesh asset importing a different
  // sub-mesh out of the same model file.
  const ezString sMeshInclude = GetTagValue(meshImportProperties, s_sMeshIncludeTags);
  const ezString sMeshExclude = GetTagValue(meshImportProperties, s_sMeshExcludeTags);

  const ezStringBuilder sMeshStem = ezPathUtils::GetFileName(sMeshAssetPath);

  ezUuid nameMatch;
  ezUuid subMeshMatch;

  for (const Candidate& candidate : candidates)
  {
    ezVariantDictionary colliderProperties;
    if (ReadMeshProperties(candidate.m_sPath, GetSubMeshPropertyNames(), colliderProperties).Failed())
      continue;

    // a collider built from a different sub-mesh is the wrong geometry, not a worse match
    if (GetTagValue(colliderProperties, s_sMeshIncludeTags) != sMeshInclude || GetTagValue(colliderProperties, s_sMeshExcludeTags) != sMeshExclude)
      continue;

    if (!subMeshMatch.IsValid())
      subMeshMatch = candidate.m_Guid;

    // Several colliders can share a sub-mesh, e.g. one simplified and one not. The one named after
    // the mesh asset is then the one generated for it.
    if (!sMeshStem.IsEmpty() && !nameMatch.IsValid() && sMeshStem.IsEqual_NoCase(ezPathUtils::GetFileName(candidate.m_sPath)))
      nameMatch = candidate.m_Guid;
  }

  if (nameMatch.IsValid())
    return nameMatch;

  return subMeshMatch;
}

ezResult ezMeshColliderUtils::ReadMeshProperties(ezStringView sAbsMeshAssetPath, ezArrayPtr<const ezStringView> properties, ezVariantDictionary& out_values)
{
  bool bWasOpen = false;
  ezDocument* pDoc = nullptr;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(sAbsMeshAssetPath, false, pTypeDesc).Succeeded())
  {
    pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsMeshAssetPath);
    bWasOpen = (pDoc != nullptr);
  }

  if (pDoc == nullptr)
    pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(sAbsMeshAssetPath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return EZ_FAILURE;

  ezResult res = EZ_FAILURE;

  if (const ezDocumentObject* pPropObj = GetColliderTopLevelObject(pDoc))
  {
    const ezIReflectedTypeAccessor& accessor = pPropObj->GetTypeAccessor();

    for (ezStringView sProperty : properties)
    {
      const ezVariant value = accessor.GetValue(sProperty);
      if (value.IsValid())
      {
        out_values.Insert(sProperty, value);
      }
    }

    res = EZ_SUCCESS;
  }

  if (!bWasOpen && !pDoc->HasWindowBeenRequested())
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  return res;
}

ezStatus ezMeshColliderUtils::CreateCollisionMesh(ezStringView sAbsColliderPath, ezEnum<ezCollisionMeshKind> kind, const ezVariantDictionary& importProperties, ezStringView sSurface, bool bOverwriteExisting, ezUuid& out_guid)
{
  out_guid = ezUuid();

  if (sAbsColliderPath.IsEmpty())
    return ezStatus("No path for the collision mesh asset was given.");

  const bool bExists = ezOSFile::ExistsFile(sAbsColliderPath);

  // CreateDocument reports an already open document through a modal message box, which would hang an
  // automated caller. Refuse here instead.
  if (bExists && !bOverwriteExisting)
    return ezStatus(ezFmt("'{}' already exists. Delete it first, or choose a different name.", sAbsColliderPath));

  // An existing collider is rewritten in place rather than deleted and created again, so that it
  // keeps its guid and anything referencing it keeps working.
  ezDocument* pDoc = bExists ? ezQtEditorApp::GetSingleton()->OpenDocument(sAbsColliderPath, ezDocumentFlags::None)
                             : ezQtEditorApp::GetSingleton()->CreateDocument(sAbsColliderPath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezStatus(ezFmt("Failed to {} collision mesh asset '{}'. Is the Jolt plugin enabled?", bExists ? "open" : "create", sAbsColliderPath));

  ezStatus result = ezStatus(EZ_SUCCESS);

  {
    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Create Collision Mesh from Mesh");

    // in a lambda, so that every failure path below cancels the transaction
    auto ApplyProperties = [&]() -> ezStatus
    {
      const ezDocumentObject* pPropObj = GetColliderTopLevelObject(pDoc);
      if (pPropObj == nullptr)
        return ezStatus("The collision mesh asset has an unexpected structure.");

      const ezRTTI* pType = pPropObj->GetTypeAccessor().GetType();

      ezHybridArray<ezStringView, 24> toWrite;
      toWrite = ezMakeArrayPtr(s_ColliderImportProperties);

      if (kind == ezCollisionMeshKind::TriangleMesh)
      {
        toWrite.PushBackRange(ezMakeArrayPtr(s_ColliderSimplificationProperties));
      }

      for (ezStringView sProperty : toWrite)
      {
        ezVariant value;
        if (!importProperties.TryGetValue(sProperty, value))
          continue;

        // the two types are matched by name, so one of them may not have this property
        if (pType->FindPropertyByName(sProperty) == nullptr)
          continue;

        ezSetObjectPropertyCommand cmd;
        cmd.m_Object = pPropObj->GetGuid();
        cmd.m_sProperty = sProperty;
        cmd.m_NewValue = value;
        EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
      }

      // "Surface" is the convex mesh's single surface. A triangle mesh has the "Surfaces" array
      // instead, which is filled from the model's material slots at transform time.
      if (!sSurface.IsEmpty() && kind == ezCollisionMeshKind::ConvexHull && pType->FindPropertyByName("Surface") != nullptr)
      {
        ezSetObjectPropertyCommand cmd;
        cmd.m_Object = pPropObj->GetGuid();
        cmd.m_sProperty = "Surface";
        cmd.m_NewValue = ezString(sSurface);
        EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
      }

      return ezStatus(EZ_SUCCESS);
    };

    result = ApplyProperties();

    if (result.Succeeded())
    {
      pHistory->FinishTransaction();
    }
    else
    {
      pHistory->CancelTransaction();
    }
  }

  if (result.Failed())
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    return result;
  }

  if (pDoc->SaveDocument(true).Failed())
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    return ezStatus(ezFmt("Failed to save collision mesh asset '{}'.", sAbsColliderPath));
  }

  out_guid = pDoc->GetGuid();

  const ezString sPath = pDoc->GetDocumentPath();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  // without this the asset is only picked up by the next file system scan
  ezFileSystemModel::GetSingleton()->NotifyOfChange(sPath);

  return ezStatus(EZ_SUCCESS);
}
