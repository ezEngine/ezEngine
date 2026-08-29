#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshColliderUtils.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

namespace
{
  constexpr ezStringView s_sTriangleExtension = "ezJoltCollisionMeshAsset"_ezsv;
  constexpr ezStringView s_sConvexExtension = "ezJoltConvexCollisionMeshAsset"_ezsv;

  /// Read alongside the import properties, but not transferred: the collision mesh asset has no
  /// equivalent, it only tells us whether there is a source file at all.
  constexpr ezStringView s_sPrimitiveType = "PrimitiveType"_ezsv;

} // namespace

ezUuid ezMeshColliderSource::GetExisting(ezEnum<ezMeshColliderKind> kind) const
{
  return (kind == ezMeshColliderKind::ConvexHull) ? m_ExistingConvexColMesh : m_ExistingTriangleColMesh;
}

bool ezMeshColliderCreator::IsMeshAsset(const ezUuid& assetGuid)
{
  return ezMeshColliderUtils::IsMeshAsset(assetGuid);
}

ezResult ezMeshColliderCreator::GatherMeshColliderSource(const ezUuid& meshAssetGuid, ezMeshColliderSource& out_source)
{
  out_source = ezMeshColliderSource();
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

  ezHybridArray<ezStringView, 24> toRead;
  toRead = ezMeshColliderUtils::GetImportPropertyNames();
  toRead.PushBackRange(ezMeshColliderUtils::GetSimplificationPropertyNames());
  toRead.PushBack(s_sPrimitiveType);

  // failing to read the properties leaves the source without a mesh file, which the caller reports
  ezMeshColliderUtils::ReadMeshProperties(sMeshAssetPath, toRead, out_source.m_ImportProperties).IgnoreResult();

  // a primitive mesh is generated procedurally, there is no model file for the collision mesh asset
  ezVariant primitiveType;
  if (out_source.m_ImportProperties.TryGetValue(s_sPrimitiveType, primitiveType))
  {
    out_source.m_bIsPrimitive = primitiveType.ConvertTo<ezInt64>() != 0; // 0 is ezMeshPrimitive::File
    out_source.m_ImportProperties.Remove(s_sPrimitiveType);
  }

  ezVariant meshFile;
  if (!out_source.m_bIsPrimitive && out_source.m_ImportProperties.TryGetValue("MeshFile"_ezsv, meshFile) && meshFile.IsA<ezString>())
  {
    out_source.m_sMeshFile = meshFile.Get<ezString>();
  }

  out_source.m_ExistingTriangleColMesh = ezMeshColliderUtils::FindExisting(ezCollisionMeshKind::TriangleMesh, out_source.m_sMeshFile, out_source.m_ImportProperties, out_source.m_sMeshAssetPath);
  out_source.m_ExistingConvexColMesh = ezMeshColliderUtils::FindExisting(ezCollisionMeshKind::ConvexHull, out_source.m_sMeshFile, out_source.m_ImportProperties, out_source.m_sMeshAssetPath);

  return EZ_SUCCESS;
}

ezString ezMeshColliderCreator::SuggestColliderPath(const ezMeshColliderSource& source, ezEnum<ezMeshColliderKind> kind, bool bAllowExisting)
{
  const ezStringView sExtension = (kind == ezMeshColliderKind::ConvexHull) ? s_sConvexExtension : s_sTriangleExtension;

  ezStringBuilder sPath = source.m_sMeshAssetPath;
  sPath.ChangeFileExtension(sExtension);

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

ezString ezMeshColliderCreator::MakeDisplayPath(ezStringView sAbsolutePath)
{
  ezStringBuilder sPath = sAbsolutePath;
  ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath);
  return sPath;
}

ezResult ezMeshColliderCreator::ResolveDisplayPath(ezStringView sPath, ezStringBuilder& out_sAbsolutePath)
{
  out_sAbsolutePath = sPath;

  if (out_sAbsolutePath.IsEmpty())
    return EZ_FAILURE;

  // the file is about to be created, so it does not exist yet
  return ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(out_sAbsolutePath, false) ? EZ_SUCCESS : EZ_FAILURE;
}

ezStatus ezMeshColliderCreator::CreateMeshCollider(const ezMeshColliderSource& source, const ezMeshColliderOptions& options)
{
  // An empty path means "wherever this collider belongs", which is what creating several at once uses.
  ezStringBuilder sColliderPath;
  if (options.m_sColliderPath.IsEmpty())
  {
    sColliderPath = SuggestColliderPath(source, options.m_Kind);
  }
  else if (ResolveDisplayPath(options.m_sColliderPath, sColliderPath).Failed())
  {
    return ezStatus(ezFmt("'{}' does not name a known data directory.", options.m_sColliderPath));
  }

  if (sColliderPath.IsEmpty())
    return ezStatus("No path for the collision mesh asset was given.");

  if (source.m_bIsPrimitive)
    return ezStatus("This mesh asset uses a procedural primitive, not a model file, so no collision mesh can be generated from it.");

  if (source.m_sMeshFile.IsEmpty())
    return ezStatus("The source file of this mesh asset could not be read, so no collision mesh can be generated from it.");

  const ezEnum<ezCollisionMeshKind> kind = (options.m_Kind == ezMeshColliderKind::ConvexHull) ? ezCollisionMeshKind::ConvexHull : ezCollisionMeshKind::TriangleMesh;

  ezUuid colliderGuid;
  EZ_SUCCEED_OR_RETURN(ezMeshColliderUtils::CreateCollisionMesh(sColliderPath, kind, source.m_ImportProperties, options.m_sSurface, options.m_bOverwriteExisting, colliderGuid));

  if (options.m_bOpenAfterCreate)
  {
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sColliderPath);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshColliderCreator::CreateMeshColliders(ezArrayPtr<const ezUuid> meshAssetGuids, const ezMeshColliderOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped)
{
  out_uiCreated = 0;
  out_uiSkipped = 0;

  // The path is decided per mesh below, so a path meant for a single collider must not leak in.
  ezMeshColliderOptions perMesh = options;
  perMesh.m_sColliderPath.Clear();

  for (const ezUuid& meshGuid : meshAssetGuids)
  {
    ezMeshColliderSource source;
    if (GatherMeshColliderSource(meshGuid, source).Failed())
    {
      // not a mesh asset - with a mixed selection this is the normal case, not a problem
      ++out_uiSkipped;
      continue;
    }

    // a collider built from the same model file counts wherever it sits, unlike the path check below
    if (!options.m_bOverwriteExisting && source.GetExisting(options.m_Kind).IsValid())
    {
      ezLog::Info("Skipping '{}': a collision mesh built from the same model file already exists.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    if (source.m_bIsPrimitive || source.m_sMeshFile.IsEmpty())
    {
      ezLog::Info("Skipping '{}': it has no model file to build a collision mesh from.", MakeDisplayPath(source.m_sMeshAssetPath));
      ++out_uiSkipped;
      continue;
    }

    // SuggestColliderPath() dodges an existing file by appending a number, which is wrong here: a
    // mesh that already has a collider is done, it should not get a second, numbered one.
    ezStringBuilder sPath = source.m_sMeshAssetPath;
    sPath.ChangeFileExtension((options.m_Kind == ezMeshColliderKind::ConvexHull) ? s_sConvexExtension : s_sTriangleExtension);

    if (!options.m_bOverwriteExisting && ezOSFile::ExistsFile(sPath))
    {
      ezLog::Info("Skipping '{}': '{}' already exists.", MakeDisplayPath(source.m_sMeshAssetPath), MakeDisplayPath(sPath));
      ++out_uiSkipped;
      continue;
    }

    perMesh.m_sColliderPath = sPath;

    // "nothing to do here" was handled above, so what is left is a real failure and stops the run
    EZ_SUCCEED_OR_RETURN(CreateMeshCollider(source, perMesh));

    ezLog::Success("Created '{}'.", MakeDisplayPath(sPath));
    ++out_uiCreated;
  }

  return ezStatus(EZ_SUCCESS);
}
