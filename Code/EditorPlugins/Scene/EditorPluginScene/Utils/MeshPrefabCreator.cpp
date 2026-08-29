#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

namespace
{
  /// Equivalent of ezSimpleAssetDocument::GetPropertyObject(), which can't be called without knowing
  /// the concrete asset type.
  const ezDocumentObject* GetTopLevelObject(const ezDocument* pDoc)
  {
    const ezDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject();
    if (pRoot == nullptr || pRoot->GetChildren().GetCount() != 1)
      return nullptr;

    return pRoot->GetChildren()[0];
  }

  /// Reads a property from an asset document without knowing its C++ type.
  ///
  /// Only closes the document again if it had to be opened here and nothing has claimed a window for
  /// it, so that a document someone else is working with is left alone.
  ezVariant ReadAssetProperty(ezStringView sAbsDocumentPath, ezStringView sProperty)
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
      return {};

    ezVariant res;
    if (const ezDocumentObject* pPropObj = GetTopLevelObject(pDoc))
    {
      res = pPropObj->GetTypeAccessor().GetValue(sProperty);
    }

    if (!bWasOpen && !pDoc->HasWindowBeenRequested())
    {
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }

    return res;
  }

  /// An asset reference is the guid in braces, which is how ezUuid already formats itself.
  ezString FormatResourceRef(const ezUuid& guid)
  {
    ezStringBuilder s;
    s.SetFormat("{}", guid);
    return s;
  }

  /// Returns an invalid uuid if the type is unknown, i.e. its plugin is not loaded.
  ezUuid AddComponent(ezCommandHistory* pHistory, const ezUuid& parentObject, ezStringView sType)
  {
    if (ezRTTI::FindTypeByName(sType) == nullptr)
      return {};

    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType(sType);
    cmd.m_Parent = parentObject;
    cmd.m_sParentProperty = "Components";

    if (pHistory->AddCommand(cmd).Failed())
      return {};

    return cmd.m_NewObjectGuid;
  }

  ezStatus SetProperty(ezCommandHistory* pHistory, const ezUuid& object, ezStringView sProperty, const ezVariant& value)
  {
    ezSetObjectPropertyCommand cmd;
    cmd.m_Object = object;
    cmd.m_sProperty = sProperty;
    cmd.m_NewValue = value;
    return pHistory->AddCommand(cmd);
  }
} // namespace

ezStringView ezMeshPrefabSource::GetDefaultRenderComponentType() const
{
  if (m_bAnimated)
    return m_LodGuids.IsEmpty() ? "ezAnimatedMeshComponent"_ezsv : "ezLodAnimatedMeshComponent"_ezsv;

  return m_LodGuids.IsEmpty() ? "ezMeshComponent"_ezsv : "ezLodMeshComponent"_ezsv;
}

bool ezMeshPrefabCreator::IsPhysicsAvailable()
{
  return ezRTTI::FindTypeByName("ezJoltStaticActorComponent") != nullptr;
}

bool ezMeshPrefabCreator::IsMeshAsset(const ezUuid& assetGuid)
{
  return ezMeshColliderUtils::IsMeshAsset(assetGuid);
}

namespace
{
  /// Collects LOD-1..N from one folder. Stops at the first gap, as LODs form a contiguous run.
  void CollectLodsFromFolder(ezStringView sFolder, ezDynamicArray<ezUuid>& out_lodGuids)
  {
    for (ezUInt32 uiLod = 1; uiLod <= ezMeshLodCreator::s_uiMaxLods; ++uiLod)
    {
      ezStringBuilder sLodName;
      sLodName.SetFormat("LOD-{}.ezMeshAsset", uiLod);

      ezStringBuilder sLodPath = sFolder;
      sLodPath.AppendPath(sLodName);

      auto pLod = ezAssetCurator::GetSingleton()->FindSubAsset(sLodPath);
      if (!pLod.isValid())
        return;

      out_lodGuids.PushBack(pLod->m_Data.m_Guid);
    }
  }

  /// Looks in the same folders that ezMeshLodCreator writes to, so that LODs it created are picked up.
  ///
  /// sMeshIncludeTags has to be passed for the same reason the creator needs it: without it a mesh
  /// that is one sub-object of a shared model file would find the LODs of a sibling sub-object.
  void FindLodSiblings(ezStringView sMeshAssetPath, ezStringView sMeshFile, ezStringView sMeshIncludeTags, ezDynamicArray<ezUuid>& out_lodGuids)
  {
    ezHybridArray<ezString, 2> folders;
    ezMeshLodCreator::GetLodFolderCandidates(sMeshAssetPath, sMeshFile, sMeshIncludeTags, folders);

    for (const ezString& sFolder : folders)
    {
      CollectLodsFromFolder(sFolder, out_lodGuids);

      if (!out_lodGuids.IsEmpty())
        return;
    }
  }
} // namespace

ezResult ezMeshPrefabCreator::GatherMeshPrefabSource(const ezUuid& meshAssetGuid, ezMeshPrefabSource& out_source)
{
  out_source = ezMeshPrefabSource();
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

    // bounds are only available once the asset has been transformed at least once
    if (const ezAssetInfoFile* pInfo = pAssetInfo->GetTransformInfo())
    {
      const ezVariant center = pInfo->GetValue(ezAssetInfoFile::Keys::BoundsCenter);
      const ezVariant extents = pInfo->GetValue(ezAssetInfoFile::Keys::BoundsHalfExtents);
      const ezVariant radius = pInfo->GetValue(ezAssetInfoFile::Keys::BoundsRadius);

      if (center.IsA<ezVec3>() && extents.IsA<ezVec3>())
      {
        out_source.m_vBoundsCenter = center.Get<ezVec3>();
        out_source.m_vBoundsHalfExtents = extents.Get<ezVec3>();
        out_source.m_fBoundsRadius = radius.IsValid() ? radius.ConvertTo<float>() : out_source.m_vBoundsHalfExtents.GetLength();
        out_source.m_bHasBounds = true;
      }
    }
  }

  const ezVariant meshFile = ReadAssetProperty(sMeshAssetPath, "MeshFile");
  if (meshFile.IsA<ezString>())
  {
    out_source.m_sMeshFile = meshFile.Get<ezString>();
  }

  ezVariantDictionary subMeshProperties;
  ezMeshColliderUtils::ReadMeshProperties(sMeshAssetPath, ezMeshColliderUtils::GetSubMeshPropertyNames(), subMeshProperties).IgnoreResult();

  ezVariant includeTags;
  subMeshProperties.TryGetValue("MeshIncludeTags", includeTags);

  FindLodSiblings(sMeshAssetPath, out_source.m_sMeshFile, includeTags.IsA<ezString>() ? includeTags.Get<ezString>().GetView() : ezStringView(), out_source.m_LodGuids);

  out_source.m_ExistingTriangleColMesh = ezMeshColliderUtils::FindExisting(ezCollisionMeshKind::TriangleMesh, out_source.m_sMeshFile, subMeshProperties, out_source.m_sMeshAssetPath);
  out_source.m_ExistingConvexColMesh = ezMeshColliderUtils::FindExisting(ezCollisionMeshKind::ConvexHull, out_source.m_sMeshFile, subMeshProperties, out_source.m_sMeshAssetPath);

  return EZ_SUCCESS;
}

ezEnum<ezMeshPrefabPhysics> ezMeshPrefabSource::GetDefaultPhysics() const
{
  // a triangle mesh is only usable for static bodies, so its existence is the more specific signal
  if (m_ExistingTriangleColMesh.IsValid())
    return ezMeshPrefabPhysics::StaticTriangleMesh;

  // a convex hull works for both; dynamic additionally needs mass and material set up sensibly
  if (m_ExistingConvexColMesh.IsValid())
    return ezMeshPrefabPhysics::StaticConvexHull;

  return ezMeshPrefabPhysics::None;
}

namespace
{
  /// Creates a collision mesh asset next to the mesh asset, or reuses a matching existing one.
  /// \see ezMeshColliderUtils
  ezStatus GetOrCreateCollisionMesh(const ezMeshPrefabSource& source, bool bConvex, ezUuid& out_guid)
  {
    const ezEnum<ezCollisionMeshKind> kind = bConvex ? ezCollisionMeshKind::ConvexHull : ezCollisionMeshKind::TriangleMesh;

    if (source.m_sMeshFile.IsEmpty())
      return ezStatus("The mesh asset has no source file, so no collision mesh can be generated from it.");

    ezHybridArray<ezStringView, 24> toRead;
    toRead = ezMeshColliderUtils::GetImportPropertyNames();
    toRead.PushBackRange(ezMeshColliderUtils::GetSimplificationPropertyNames());

    ezVariantDictionary importProperties;
    ezMeshColliderUtils::ReadMeshProperties(source.m_sMeshAssetPath, toRead, importProperties).IgnoreResult();

    // A matching collider counts wherever it sits, so this finds more than the path check below.
    out_guid = ezMeshColliderUtils::FindExisting(kind, source.m_sMeshFile, importProperties, source.m_sMeshAssetPath);
    if (out_guid.IsValid())
      return ezStatus(EZ_SUCCESS);

    ezStringBuilder sColMeshPath = source.m_sMeshAssetPath;
    sColMeshPath.ChangeFileExtension(ezMeshColliderUtils::GetExtension(kind));

    if (ezOSFile::ExistsFile(sColMeshPath))
    {
      auto pExisting = ezAssetCurator::GetSingleton()->FindSubAsset(sColMeshPath);
      if (pExisting.isValid())
      {
        out_guid = pExisting->m_Data.m_Guid;
        return ezStatus(EZ_SUCCESS);
      }

      return ezStatus(ezFmt("'{}' already exists but is not a collision mesh asset.", sColMeshPath));
    }

    // the mesh file is what the collider is built from
    if (!importProperties.Contains("MeshFile"_ezsv))
    {
      importProperties.Insert("MeshFile"_ezsv, ezVariant(source.m_sMeshFile));
    }

    // no surface here: a prefab's collider gets whatever the shape component specifies
    return ezMeshColliderUtils::CreateCollisionMesh(sColMeshPath, kind, importProperties, {}, false, out_guid);
  }
} // namespace

ezString ezMeshPrefabCreator::SuggestPrefabPath(const ezMeshPrefabSource& source, bool bAllowExisting)
{
  ezStringBuilder sPath = source.m_sMeshAssetPath;
  sPath.ChangeFileExtension("ezPrefab");

  if (bAllowExisting || !ezOSFile::ExistsFile(sPath))
    return sPath;

  const ezString sBaseName = ezPathUtils::GetFileName(sPath);

  for (ezUInt32 i = 2; i < 100; ++i)
  {
    ezStringBuilder sCandidateName;
    sCandidateName.SetFormat("{}{}", sBaseName, i);

    ezStringBuilder sCandidate = sPath;
    sCandidate.ChangeFileName(sCandidateName);

    if (!ezOSFile::ExistsFile(sCandidate))
      return sCandidate;
  }

  return sPath;
}

ezString ezMeshPrefabCreator::MakeDisplayPath(ezStringView sAbsolutePath)
{
  ezStringBuilder sPath = sAbsolutePath;
  ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath);
  return sPath;
}

ezResult ezMeshPrefabCreator::ResolveDisplayPath(ezStringView sPath, ezStringBuilder& out_sAbsolutePath)
{
  out_sAbsolutePath = sPath;

  if (out_sAbsolutePath.IsEmpty())
    return EZ_FAILURE;

  // the file is about to be created, so it does not exist yet
  return ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(out_sAbsolutePath, false) ? EZ_SUCCESS : EZ_FAILURE;
}

ezStatus ezMeshPrefabCreator::CreateMeshPrefab(const ezMeshPrefabSource& source, const ezMeshPrefabOptions& options)
{
  // An empty path means "wherever this prefab belongs", which is what creating several at once uses.
  ezStringBuilder sPrefabPath;
  if (options.m_sPrefabPath.IsEmpty())
  {
    sPrefabPath = SuggestPrefabPath(source);
  }
  else if (ResolveDisplayPath(options.m_sPrefabPath, sPrefabPath).Failed())
  {
    return ezStatus(ezFmt("'{}' does not name a known data directory.", options.m_sPrefabPath));
  }

  if (sPrefabPath.IsEmpty())
    return ezStatus("No prefab path was given.");

  const bool bExists = ezOSFile::ExistsFile(sPrefabPath);

  // CreateDocument reports an already open document through a modal message box, which would hang an
  // automated caller. Refuse here instead.
  if (bExists && !options.m_bOverwriteExisting)
  {
    return ezStatus(ezFmt("'{}' already exists. Delete it first, or choose a different name.", sPrefabPath));
  }

  const bool bWantsPhysics = options.m_Physics != ezMeshPrefabPhysics::None;
  const bool bConvex = options.m_Physics == ezMeshPrefabPhysics::StaticConvexHull || options.m_Physics == ezMeshPrefabPhysics::DynamicConvexHull;
  const bool bDynamic = options.m_Physics == ezMeshPrefabPhysics::DynamicConvexHull || options.m_Physics == ezMeshPrefabPhysics::DynamicBox;
  const bool bBoxShape = options.m_Physics == ezMeshPrefabPhysics::StaticBox || options.m_Physics == ezMeshPrefabPhysics::DynamicBox;

  if (bWantsPhysics && !IsPhysicsAvailable())
    return ezStatus("Physics components are not available. Enable the Jolt plugin in the project settings.");

  if (bBoxShape && !source.m_bHasBounds)
    return ezStatus("The mesh bounds are unknown. Transform the mesh asset first, or use a collision mesh instead of a box.");

  // first, so that a failure here doesn't leave a half-built prefab behind
  ezUuid colMeshGuid;
  if (bWantsPhysics && !bBoxShape)
  {
    EZ_SUCCEED_OR_RETURN(GetOrCreateCollisionMesh(source, bConvex, colMeshGuid));
  }

  // An existing prefab is rewritten in place rather than deleted and created again, so that it keeps
  // its guid and anything referencing it keeps working.
  ezDocument* pDoc = bExists ? ezQtEditorApp::GetSingleton()->OpenDocument(sPrefabPath, ezDocumentFlags::None)
                             : ezQtEditorApp::GetSingleton()->CreateDocument(sPrefabPath, ezDocumentFlags::None);

  if (pDoc == nullptr)
    return ezStatus(ezFmt("Failed to {} prefab document '{}'.", bExists ? "open" : "create", sPrefabPath));

  ezStatus result = ezStatus(EZ_SUCCESS);

  {
    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Create Prefab from Mesh");

    // in a lambda, so that every failure path below cancels the transaction
    auto BuildPrefab = [&]() -> ezStatus
    {
      // A new prefab is not empty: the document manager clones a template that provides the root
      // object. A second one would make the prefab invalid.
      ezUuid rootObject;
      for (const ezDocumentObject* pChild : pDoc->GetObjectManager()->GetRootObject()->GetChildren())
      {
        if (pChild->GetParentProperty() == "Children"_ezsv)
        {
          rootObject = pChild->GetGuid();
          break;
        }
      }

      if (!rootObject.IsValid())
      {
        ezAddObjectCommand cmd;
        cmd.m_Index = -1;
        cmd.SetType("ezGameObject");
        cmd.m_sParentProperty = "Children";
        EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
        rootObject = cmd.m_NewObjectGuid;
      }

      // this exact name is what marks the object as the prefab root
      EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, rootObject, "Name", "<Prefab-Root>"));

      // a prefab that is being rewritten still holds the components and children it had
      if (const ezDocumentObject* pRoot = pDoc->GetObjectManager()->GetObject(rootObject))
      {
        ezHybridArray<ezUuid, 16> toRemove;
        for (const ezDocumentObject* pChild : pRoot->GetChildren())
        {
          toRemove.PushBack(pChild->GetGuid());
        }

        for (const ezUuid& guid : toRemove)
        {
          ezRemoveObjectCommand remove;
          remove.m_Object = guid;
          EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(remove));
        }
      }

      {
        const ezString sRenderType = options.m_sRenderComponentType.IsEmpty() ? ezString(source.GetDefaultRenderComponentType()) : options.m_sRenderComponentType;

        const ezUuid renderComponent = AddComponent(pHistory, rootObject, sRenderType);
        if (!renderComponent.IsValid())
          return ezStatus(ezFmt("Failed to add component '{}'.", sRenderType));

        // The two LOD components are separate types with identical properties, one skinned and one not.
        const bool bLodComponent = (sRenderType == "ezLodMeshComponent") || (sRenderType == "ezLodAnimatedMeshComponent");

        if (bLodComponent)
        {
          // LOD 0 is the mesh asset itself, the gathered guids continue from LOD 1
          ezHybridArray<ezUuid, 8> allLods;
          allLods.PushBack(source.m_MeshAssetGuid);
          allLods.PushBackRange(source.m_LodGuids);

          // Screen coverage fractions, not distances: the coverage below which the component switches
          // away from that LOD. Even a nearby model covers little of the screen, hence the small
          // values. The last LOD gets 0, so that it is used out to the horizon.
          const float fThresholds[] = {0.2f, 0.1f, 0.05f, 0.02f};

          for (ezUInt32 i = 0; i < allLods.GetCount(); ++i)
          {
            ezAddObjectCommand cmd;
            cmd.m_Index = (ezInt32)i;
            cmd.SetType((sRenderType == "ezLodAnimatedMeshComponent") ? "ezLodAnimatedMeshLod" : "ezLodMeshLod");
            cmd.m_Parent = renderComponent;
            cmd.m_sParentProperty = "Meshes";
            EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));

            // halving past the end of the table keeps the values strictly decreasing, which the
            // component needs to switch between LODs
            const bool bLastLod = (i + 1 == allLods.GetCount());

            float fThreshold = 0.0f;
            if (!bLastLod)
            {
              fThreshold = fThresholds[EZ_ARRAY_SIZE(fThresholds) - 1];

              for (ezUInt32 uiExtra = EZ_ARRAY_SIZE(fThresholds); uiExtra <= i; ++uiExtra)
              {
                fThreshold *= 0.5f;
              }

              if (i < EZ_ARRAY_SIZE(fThresholds))
              {
                fThreshold = fThresholds[i];
              }
            }

            EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, cmd.m_NewObjectGuid, "Mesh", FormatResourceRef(allLods[i])));
            EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, cmd.m_NewObjectGuid, "Threshold", fThreshold));
          }

          // the component culls by these rather than deriving them from the meshes
          if (source.m_bHasBounds)
          {
            EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, renderComponent, "BoundsOffset", source.m_vBoundsCenter));
            EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, renderComponent, "BoundsRadius", ezMath::Clamp(source.m_fBoundsRadius, 0.01f, 100.0f)));
          }
        }
        else
        {
          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, renderComponent, "Mesh", FormatResourceRef(source.m_MeshAssetGuid)));
        }
      }

      if (bWantsPhysics)
      {
        const ezStringView sActorType = bDynamic ? "ezJoltDynamicActorComponent"_ezsv : "ezJoltStaticActorComponent"_ezsv;

        const ezUuid actorComponent = AddComponent(pHistory, rootObject, sActorType);
        if (!actorComponent.IsValid())
          return ezStatus(ezFmt("Failed to add component '{}'.", sActorType));

        EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, actorComponent, "CollisionLayer", options.m_uiCollisionLayer));

        if (!options.m_sSurfaceAsset.IsEmpty())
        {
          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, actorComponent, "Surface", options.m_sSurfaceAsset));
        }

        if (bBoxShape)
        {
          // a child object, so that the box can be offset to the mesh bounds centre
          ezUuid shapeObject;
          {
            ezAddObjectCommand cmd;
            cmd.m_Index = -1;
            cmd.SetType("ezGameObject");
            cmd.m_Parent = rootObject;
            cmd.m_sParentProperty = "Children";
            EZ_SUCCEED_OR_RETURN(pHistory->AddCommand(cmd));
            shapeObject = cmd.m_NewObjectGuid;
          }

          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeObject, "Name", "Collider"));
          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeObject, "LocalPosition", source.m_vBoundsCenter));

          const ezUuid shapeComponent = AddComponent(pHistory, shapeObject, "ezJoltShapeBoxComponent");
          if (!shapeComponent.IsValid())
            return ezStatus("Failed to add component 'ezJoltShapeBoxComponent'.");

          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeComponent, "HalfExtents", source.m_vBoundsHalfExtents));
        }
        else if (bConvex)
        {
          const ezUuid shapeComponent = AddComponent(pHistory, rootObject, "ezJoltShapeConvexHullComponent");
          if (!shapeComponent.IsValid())
            return ezStatus("Failed to add component 'ezJoltShapeConvexHullComponent'.");

          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, shapeComponent, "CollisionMesh", FormatResourceRef(colMeshGuid)));
        }
        else
        {
          // a triangle mesh is referenced by the actor directly, no shape component involved
          EZ_SUCCEED_OR_RETURN(SetProperty(pHistory, actorComponent, "CollisionMesh", FormatResourceRef(colMeshGuid)));
        }
      }

      return ezStatus(EZ_SUCCESS);
    };

    result = BuildPrefab();

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
    return ezStatus(ezFmt("Failed to save prefab '{}'.", sPrefabPath));
  }

  const ezString sPath = pDoc->GetDocumentPath();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  ezFileSystemModel::GetSingleton()->NotifyOfChange(sPath);

  if (options.m_bOpenAfterCreate)
  {
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sPath);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshPrefabCreator::CreateMeshPrefabs(ezArrayPtr<const ezUuid> meshAssetGuids, const ezMeshPrefabOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  const bool bWantsPhysics = options.m_Physics != ezMeshPrefabPhysics::None;
  const bool bBoxShape = options.m_Physics == ezMeshPrefabPhysics::StaticBox || options.m_Physics == ezMeshPrefabPhysics::DynamicBox;

  if (bWantsPhysics && !IsPhysicsAvailable())
    return ezStatus("Physics components are not available. Enable the Jolt plugin in the project settings.");

  for (const ezUuid& meshGuid : meshAssetGuids)
  {
    ezMeshPrefabSource source;
    if (GatherMeshPrefabSource(meshGuid, source).Failed())
    {
      // not a mesh asset - with a mixed selection this is the normal case, not a problem
      ++out_uiSkipped;
      continue;
    }

    // SuggestPrefabPath() dodges an existing file by appending a number, which is wrong here: a mesh
    // that already has a prefab is done, it should not get a second, numbered one.
    ezStringBuilder sPath = source.m_sMeshAssetPath;
    sPath.ChangeFileExtension("ezPrefab");

    if (!options.m_bOverwriteExisting && ezOSFile::ExistsFile(sPath))
    {
      ezLog::Info("Skipping '{}': '{}' already exists.", MakeDisplayPath(source.m_sMeshAssetPath), MakeDisplayPath(sPath));
      ++out_uiSkipped;
      continue;
    }

    if (bBoxShape && !source.m_bHasBounds)
    {
      ezLog::Info("Skipping '{}': its bounds are unknown, so no box collider can be sized. Transform the mesh asset first.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    if (bWantsPhysics && !bBoxShape && source.m_sMeshFile.IsEmpty())
    {
      ezLog::Info("Skipping '{}': it has no model file to build a collision mesh from.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    ezMeshPrefabOptions perMesh = options;

    // the path was just checked to be free, so it is passed on explicitly
    perMesh.m_sPrefabPath = sPath;

    // a selection can mix animated and static meshes, so the component type is decided per mesh
    perMesh.m_sRenderComponentType.Clear();

    // "nothing to do here" was handled above, so what is left is a real failure and stops the run
    EZ_SUCCEED_OR_RETURN(CreateMeshPrefab(source, perMesh));

    ezLog::Success("Created '{}'.", MakeDisplayPath(sPath));
    ++out_uiCreated;
  }

  return ezStatus(EZ_SUCCESS);
}
