#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

namespace
{
  /// The mesh asset properties that a LOD asset has to share with the mesh it is a LOD of, so that
  /// it describes the same geometry in the same place, only with fewer triangles.
  constexpr ezStringView s_ImportProperties[] = {
    "MeshFile"_ezsv,
    "MeshIncludeTags"_ezsv,
    "MeshExcludeTags"_ezsv,
    "ImportTransform"_ezsv,
    "RightDir"_ezsv,
    "UpDir"_ezsv,
    "FlipForwardDir"_ezsv,
    "PositionOffset"_ezsv,
    "UniformScaling"_ezsv,
    "RecalculateNormals"_ezsv,
    "RecalculateTangents"_ezsv,
    "HighPrecision"_ezsv,
    "VertexColorConversion"_ezsv,
    "ImportMaterials"_ezsv,
    "NormalWeight"_ezsv,
    "AggressiveSimplification"_ezsv,
  };

  /// Read alongside the import properties, but handled separately: they decide whether LODs can be
  /// made at all and where the ladder starts.
  constexpr ezStringView s_sPrimitiveType = "PrimitiveType"_ezsv;
  constexpr ezStringView s_sSimplifyMesh = "SimplifyMesh"_ezsv;
  constexpr ezStringView s_sMeshSimplification = "MeshSimplification"_ezsv;

  /// Equivalent of ezSimpleAssetDocument::GetPropertyObject(), which can't be called without knowing
  /// the concrete asset type.
  const ezDocumentObject* GetTopLevelObject(const ezDocument* pDoc)
  {
    const ezDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject();
    if (pRoot == nullptr || pRoot->GetChildren().GetCount() != 1)
      return nullptr;

    return pRoot->GetChildren()[0];
  }

  /// Reads the mesh asset's import settings and material slots without knowing its C++ type.
  ///
  /// Only closes the document again if it had to be opened here and nothing has claimed a window for
  /// it, so that a document someone else is working with is left alone.
  ezResult ReadMeshAsset(ezStringView sAbsDocumentPath, ezMeshLodSource& ref_source)
  {
    bool bWasOpen = false;
    ezDocument* pDoc = nullptr;

    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (ezDocumentManager::FindDocumentTypeFromPath(sAbsDocumentPath, false, pTypeDesc).Succeeded())
    {
      pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsDocumentPath);
      bWasOpen = (pDoc != nullptr);
    }

    if (pDoc == nullptr)
      pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(sAbsDocumentPath, ezDocumentFlags::None);

    if (pDoc == nullptr)
      return EZ_FAILURE;

    ezResult res = EZ_FAILURE;

    if (const ezDocumentObject* pPropObj = GetTopLevelObject(pDoc))
    {
      const ezIReflectedTypeAccessor& accessor = pPropObj->GetTypeAccessor();

      for (ezStringView sProperty : s_ImportProperties)
      {
        const ezVariant value = accessor.GetValue(sProperty);
        if (value.IsValid())
        {
          ref_source.m_ImportProperties.Insert(sProperty, value);
        }
      }

      // a primitive is generated procedurally, there is no geometry to simplify
      const ezVariant primitiveType = accessor.GetValue(s_sPrimitiveType);
      ref_source.m_bIsPrimitive = primitiveType.IsValid() && primitiveType.ConvertTo<ezInt64>() != 0; // 0 is ezMeshPrimitive::File

      const ezVariant meshFile = accessor.GetValue("MeshFile"_ezsv);
      if (!ref_source.m_bIsPrimitive && meshFile.IsA<ezString>())
      {
        ref_source.m_sMeshFile = meshFile.Get<ezString>();
      }

      // Where the LOD ladder starts. A mesh that does not simplify at all starts from the full model,
      // no matter what value the (then unused) property happens to hold.
      const ezVariant bSimplify = accessor.GetValue(s_sSimplifyMesh);
      const ezVariant uiSimplification = accessor.GetValue(s_sMeshSimplification);

      if (bSimplify.IsValid() && bSimplify.ConvertTo<bool>() && uiSimplification.IsValid())
      {
        ref_source.m_uiBaseSimplification = (ezUInt8)ezMath::Clamp<ezInt64>(uiSimplification.ConvertTo<ezInt64>(), 0, 99);
      }

      // the LODs render with the same materials as the mesh, so the slots are copied rather than re-imported
      const ezInt32 iSlots = accessor.GetCount("Materials"_ezsv);
      for (ezInt32 i = 0; i < iSlots; ++i)
      {
        const ezVariant slotGuid = accessor.GetValue("Materials"_ezsv, i);
        if (!slotGuid.IsA<ezUuid>())
          continue;

        const ezDocumentObject* pSlot = pDoc->GetObjectManager()->GetObject(slotGuid.Get<ezUuid>());
        if (pSlot == nullptr)
          continue;

        ezVariantDictionary& slot = ref_source.m_MaterialSlots.ExpandAndGetRef();
        slot.Insert("Label", pSlot->GetTypeAccessor().GetValue("Label"_ezsv));
        slot.Insert("Resource", pSlot->GetTypeAccessor().GetValue("Resource"_ezsv));
      }

      res = EZ_SUCCESS;
    }

    if (!bWasOpen && !pDoc->HasWindowBeenRequested())
    {
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }

    return res;
  }

  /// An existing folder is preferred over inventing a second one next to it, so that a mesh that was
  /// renamed after its import keeps writing into the folder its LODs are already in.
  ezString DetermineLodFolder(ezStringView sMeshAssetPath, ezStringView sMeshFile)
  {
    ezHybridArray<ezString, 2> candidates;
    ezMeshLodCreator::GetLodFolderCandidates(sMeshAssetPath, sMeshFile, candidates);

    for (const ezString& sPath : candidates)
    {
      if (ezOSFile::ExistsDirectory(sPath))
        return sPath;
    }

    // none exists yet, so the mesh asset's own name decides - which is what a fresh import would use
    return candidates.IsEmpty() ? ezString() : candidates[0];
  }
} // namespace

void ezMeshLodCreator::GetLodFolderCandidates(ezStringView sMeshAssetPath, ezStringView sMeshFile, ezDynamicArray<ezString>& out_folders)
{
  out_folders.Clear();

  ezStringBuilder sDir = sMeshAssetPath;
  sDir.PathParentDirectory();

  ezHybridArray<ezStringView, 2> names;
  names.PushBack(ezPathUtils::GetFileName(sMeshAssetPath));

  if (!sMeshFile.IsEmpty())
  {
    const ezStringView sSourceName = ezPathUtils::GetFileName(sMeshFile);
    if (sSourceName != names[0])
    {
      names.PushBack(sSourceName);
    }
  }

  for (ezStringView sName : names)
  {
    ezStringBuilder sFolderName;
    sFolderName.SetFormat("{}_data", sName);

    ezStringBuilder sPath = sDir;
    sPath.AppendPath(sFolderName);

    out_folders.PushBack(sPath);
  }
}

bool ezMeshLodSource::HasLod(ezUInt32 uiLod) const
{
  if (uiLod == 0 || uiLod > m_ExistingLods.GetCount())
    return false;

  return m_ExistingLods[uiLod - 1].IsValid();
}

bool ezMeshLodCreator::IsMeshAsset(const ezUuid& assetGuid)
{
  return ezMeshColliderUtils::IsMeshAsset(assetGuid);
}

ezUInt8 ezMeshLodCreator::GetLodSimplification(ezUInt8 uiBaseSimplification, ezUInt32 uiLod)
{
  // the value is the percentage of triangles removed, so halving what is left each time is the
  // midpoint between the previous level and 100
  float fSimplification = ezMath::Clamp<float>(uiBaseSimplification, 0.0f, 99.0f);

  for (ezUInt32 i = 0; i < uiLod; ++i)
  {
    fSimplification += (100.0f - fSimplification) * 0.5f;
  }

  // 100 would remove the whole mesh, and the importer clamps to 99 anyway
  return (ezUInt8)ezMath::Clamp<ezInt32>((ezInt32)(fSimplification + 0.5f), 1, 99);
}

ezUInt8 ezMeshLodCreator::GetLodSimplificationError(ezUInt32 uiLod)
{
  // What the mesh import uses for its own LODs. A more distant mesh can afford a coarser silhouette,
  // and the last entry repeats for levels past the table.
  constexpr ezUInt8 uiErrors[] = {5, 5, 10, 15};

  if (uiLod == 0)
    return uiErrors[0];

  return uiErrors[ezMath::Min<ezUInt32>(uiLod - 1, EZ_ARRAY_SIZE(uiErrors) - 1)];
}

ezString ezMeshLodCreator::GetLodPath(const ezMeshLodSource& source, ezUInt32 uiLod)
{
  ezStringBuilder sName;
  sName.SetFormat("LOD-{}.ezMeshAsset", uiLod);

  ezStringBuilder sPath = source.m_sLodFolder;
  sPath.AppendPath(sName);
  return sPath;
}

ezResult ezMeshLodCreator::GatherMeshLodSource(const ezUuid& meshAssetGuid, ezMeshLodSource& out_source)
{
  out_source = ezMeshLodSource();
  out_source.m_MeshAssetGuid = meshAssetGuid;

  ezStringBuilder sMeshAssetPath;

  // the curator lock must not be held while documents are opened further below
  {
    auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(meshAssetGuid);
    if (!pSubAsset.isValid() || pSubAsset->m_pAssetInfo == nullptr)
      return EZ_FAILURE;

    const ezAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;
    if (pAssetInfo->m_pDocumentTypeDescriptor == nullptr)
      return EZ_FAILURE;

    const ezStringView sDocType = pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;
    if (sDocType != ezMeshColliderUtils::s_sMeshDocType && sDocType != ezMeshColliderUtils::s_sAnimatedMeshDocType)
      return EZ_FAILURE;

    out_source.m_bAnimated = (sDocType == ezMeshColliderUtils::s_sAnimatedMeshDocType);
    sMeshAssetPath = pAssetInfo->m_Path.GetAbsolutePath();
    out_source.m_sMeshAssetPath = sMeshAssetPath;
  }

  // a mesh asset that cannot be read leaves the source without a mesh file, which the caller reports
  ReadMeshAsset(sMeshAssetPath, out_source).IgnoreResult();

  out_source.m_sLodFolder = DetermineLodFolder(sMeshAssetPath, out_source.m_sMeshFile);

  for (ezUInt32 uiLod = 1; uiLod <= s_uiMaxLods; ++uiLod)
  {
    const ezString sPath = GetLodPath(out_source, uiLod);

    auto pLod = ezAssetCurator::GetSingleton()->FindSubAsset(sPath);
    out_source.m_ExistingLods.PushBack(pLod.isValid() ? pLod->m_Data.m_Guid : ezUuid());
  }

  // trailing gaps say nothing, only the ones between existing LODs matter
  while (!out_source.m_ExistingLods.IsEmpty() && !out_source.m_ExistingLods.PeekBack().IsValid())
  {
    out_source.m_ExistingLods.PopBack();
  }

  return EZ_SUCCESS;
}

ezStatus ezMeshLodCreator::CreateMeshLods(const ezMeshLodSource& source, const ezMeshLodOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  if (source.m_bIsPrimitive)
    return ezStatus("This mesh asset uses a procedural primitive, not a model file, so no LODs can be generated from it.");

  if (source.m_sMeshFile.IsEmpty())
    return ezStatus("The source file of this mesh asset could not be read, so no LODs can be generated from it.");

  if (source.m_sLodFolder.IsEmpty())
    return ezStatus("The folder for the LOD assets could not be determined.");

  const ezUInt32 uiLodCount = ezMath::Min(options.m_uiLodCount, s_uiMaxLods);
  if (uiLodCount == 0)
    return ezStatus("No LODs were requested.");

  // The LOD sub-folder usually does not exist yet. Creating a document below a missing folder fails
  // through a modal message box, which would hang an automated caller.
  if (ezOSFile::CreateDirectoryStructure(source.m_sLodFolder).Failed())
    return ezStatus(ezFmt("Failed to create the folder '{}'.", source.m_sLodFolder));

  ezHybridArray<ezString, 4> created;

  for (ezUInt32 uiLod = 1; uiLod <= uiLodCount; ++uiLod)
  {
    const ezString sPath = GetLodPath(source, uiLod);

    const bool bExists = ezOSFile::ExistsFile(sPath);

    // An existing LOD may have been tuned by hand, so replacing it has to be asked for.
    if (bExists && !options.m_bOverwriteExisting)
    {
      ezLog::Info("Skipping '{}': it already exists.", sPath);
      ++out_uiSkipped;
      continue;
    }

    // An existing LOD is rewritten in place rather than deleted and created again, so that it keeps
    // its guid and anything referencing it keeps working.
    ezDocument* pDoc = bExists ? ezQtEditorApp::GetSingleton()->OpenDocument(sPath, ezDocumentFlags::None)
                               : ezQtEditorApp::GetSingleton()->CreateDocument(sPath, ezDocumentFlags::None);

    if (pDoc == nullptr)
      return ezStatus(ezFmt("Failed to {} LOD asset '{}'.", bExists ? "open" : "create", sPath));

    ezStatus result = ezStatus(EZ_SUCCESS);

    {
      auto pHistory = pDoc->GetCommandHistory();
      pHistory->StartTransaction("Create LOD from Mesh");

      // in a lambda, so that every failure path below cancels the transaction
      auto ApplyProperties = [&]() -> ezStatus
      {
        const ezDocumentObject* pPropObj = GetTopLevelObject(pDoc);
        if (pPropObj == nullptr)
          return ezStatus("The mesh asset has an unexpected structure.");

        const ezRTTI* pType = pPropObj->GetTypeAccessor().GetType();

        auto SetProperty = [&](ezStringView sProperty, const ezVariant& value) -> ezStatus
        {
          ezSetObjectPropertyCommand cmd;
          cmd.m_Object = pPropObj->GetGuid();
          cmd.m_sProperty = sProperty;
          cmd.m_NewValue = value;
          return pHistory->AddCommand(cmd);
        };

        for (ezStringView sProperty : s_ImportProperties)
        {
          ezVariant value;
          if (!source.m_ImportProperties.TryGetValue(sProperty, value))
            continue;

          // a mesh and an animated mesh asset do not have exactly the same properties
          if (pType->FindPropertyByName(sProperty) == nullptr)
            continue;

          EZ_SUCCEED_OR_RETURN(SetProperty(sProperty, value));
        }

        // what makes this a LOD rather than a copy of the mesh
        EZ_SUCCEED_OR_RETURN(SetProperty(s_sSimplifyMesh, true));
        EZ_SUCCEED_OR_RETURN(SetProperty(s_sMeshSimplification, GetLodSimplification(source.m_uiBaseSimplification, uiLod)));
        EZ_SUCCEED_OR_RETURN(SetProperty("MaxSimplificationError"_ezsv, GetLodSimplificationError(uiLod)));

        // The LODs share the mesh's materials rather than importing their own, which would create a
        // second set of material assets for the same model.
        EZ_SUCCEED_OR_RETURN(SetProperty("ImportMaterials"_ezsv, false));

        // a LOD that is being rewritten still holds the slots it had
        for (ezInt32 i = pPropObj->GetTypeAccessor().GetCount("Materials"_ezsv) - 1; i >= 0; --i)
        {
          const ezVariant slotGuid = pPropObj->GetTypeAccessor().GetValue("Materials"_ezsv, i);
          if (!slotGuid.IsA<ezUuid>())
            continue;

          ezRemoveObjectCommand remove;
          remove.m_Object = slotGuid.Get<ezUuid>();
          EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(remove));
        }

        for (ezUInt32 i = 0; i < source.m_MaterialSlots.GetCount(); ++i)
        {
          ezAddObjectCommand add;
          add.m_Index = (ezInt32)i;
          add.m_pType = ezGetStaticRTTI<ezMaterialResourceSlot>();
          add.m_Parent = pPropObj->GetGuid();
          add.m_sParentProperty = "Materials";
          EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(add));

          for (auto it : source.m_MaterialSlots[i])
          {
            ezSetObjectPropertyCommand cmd;
            cmd.m_Object = add.m_NewObjectGuid;
            cmd.m_sProperty = it.Key();
            cmd.m_NewValue = it.Value();
            EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
          }
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
      return ezStatus(ezFmt("Failed to save LOD asset '{}'.", sPath));
    }

    const ezString sSavedPath = pDoc->GetDocumentPath();
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

    // no '%' sign: a literal percent in an ezLog format string is consumed as a format spec
    ezLog::Success("Created '{}' at {} percent simplification.", sSavedPath, (ezUInt32)GetLodSimplification(source.m_uiBaseSimplification, uiLod));
    created.PushBack(sSavedPath);
    ++out_uiCreated;
  }

  // Only once every document is written and closed: notifying the curator makes it look at the
  // folder, which would re-enter this code between two LODs of the same mesh.
  for (const ezString& sPath : created)
  {
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
  }

  if (options.m_bOpenAfterCreate)
  {
    for (const ezString& sPath : created)
    {
      ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sPath);
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshLodCreator::CreateMeshLodsForAll(ezArrayPtr<const ezUuid> meshAssetGuids, const ezMeshLodOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  // opening the created documents is left to the caller, which may not want that many tabs
  ezMeshLodOptions perMesh = options;

  for (const ezUuid& meshGuid : meshAssetGuids)
  {
    ezMeshLodSource source;
    if (GatherMeshLodSource(meshGuid, source).Failed())
    {
      // not a mesh asset - with a mixed selection this is the normal case, not a problem
      ++out_uiSkipped;
      continue;
    }

    // A mesh that is already a LOD must not get LODs of its own, or the folders nest without end.
    if (ezPathUtils::GetFileName(source.m_sMeshAssetPath).StartsWith_NoCase("LOD-"))
    {
      ezLog::Info("Skipping '{}': it is itself a LOD.", source.m_sMeshAssetPath);
      ++out_uiSkipped;
      continue;
    }

    if (source.m_bIsPrimitive || source.m_sMeshFile.IsEmpty())
    {
      ezLog::Info("Skipping '{}': it has no model file to build LODs from.", source.m_sMeshAssetPath);
      ++out_uiSkipped;
      continue;
    }

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;

    // "nothing to do here" was handled above, so what is left is a real failure and stops the run
    EZ_SUCCEED_OR_RETURN(CreateMeshLods(source, perMesh, uiCreated, uiSkipped));

    out_uiCreated += uiCreated;
    out_uiSkipped += uiSkipped;
  }

  return ezStatus(EZ_SUCCESS);
}
