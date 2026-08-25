#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/McpTools/MeshPrefabTool.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpMeshPrefabTool, 1, ezRTTIDefaultAllocator<ezMcpMeshPrefabTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  struct PhysicsName
  {
    ezStringView m_sName;
    ezMeshPrefabPhysics::Enum m_Value;
  };

  /// The spellings that the 'physics' argument accepts, and that mesh_prefab_info reports back.
  constexpr PhysicsName s_PhysicsNames[] = {
    {"None"_ezsv, ezMeshPrefabPhysics::None},
    {"StaticTriangleMesh"_ezsv, ezMeshPrefabPhysics::StaticTriangleMesh},
    {"StaticConvexHull"_ezsv, ezMeshPrefabPhysics::StaticConvexHull},
    {"StaticBox"_ezsv, ezMeshPrefabPhysics::StaticBox},
    {"DynamicConvexHull"_ezsv, ezMeshPrefabPhysics::DynamicConvexHull},
    {"DynamicBox"_ezsv, ezMeshPrefabPhysics::DynamicBox},
  };

  ezStringView PhysicsToString(ezMeshPrefabPhysics::Enum value)
  {
    for (const PhysicsName& n : s_PhysicsNames)
    {
      if (n.m_Value == value)
        return n.m_sName;
    }

    return "None"_ezsv;
  }

  /// Case insensitive, because the argument comes from a language model reading the schema.
  bool PhysicsFromString(ezStringView sName, ezMeshPrefabPhysics::Enum& out_value)
  {
    for (const PhysicsName& n : s_PhysicsNames)
    {
      if (n.m_sName.IsEqual_NoCase(sName))
      {
        out_value = n.m_Value;
        return true;
      }
    }

    return false;
  }

  /// Whether a mode can be used for this mesh, and why not if it can't.
  ///
  /// The box modes are sized from the bounds, which only exist once the asset was transformed. The
  /// collision mesh modes generate an asset from the source model file, so they need that path.
  ezStringView GetPhysicsUnavailableReason(const ezMeshPrefabSource& source, ezMeshPrefabPhysics::Enum mode)
  {
    if (mode == ezMeshPrefabPhysics::None)
      return {};

    if (!ezMeshPrefabCreator::IsPhysicsAvailable())
      return "The Jolt plugin is not loaded, so no collider components exist."_ezsv;

    switch (mode)
    {
      case ezMeshPrefabPhysics::StaticBox:
      case ezMeshPrefabPhysics::DynamicBox:
        if (!source.m_bHasBounds)
          return "The mesh asset has no bounds. Transform it first, with asset_transform."_ezsv;
        break;

      case ezMeshPrefabPhysics::StaticTriangleMesh:
      case ezMeshPrefabPhysics::StaticConvexHull:
      case ezMeshPrefabPhysics::DynamicConvexHull:
        if (source.m_sMeshFile.IsEmpty())
          return "The mesh asset has no source model file, so no collision mesh asset can be generated from it."_ezsv;
        break;

      default:
        break;
    }

    return {};
  }

  /// Turns whatever the caller passed into an absolute path.
  ///
  /// Same three spellings the document tools accept, except that the file must not exist yet, so the
  /// data directory lookup cannot go by existence.
  ///
  /// The result is always absolute, as everything downstream asserts on relative paths. A spelling
  /// that resolves to no data directory is interpreted relative to the project directory.
  ezStringBuilder ResolveOutputPath(ezStringView sPath)
  {
    ezStringBuilder sResult = sPath;
    sResult.MakeCleanPath();

    if (sResult.IsEmpty() || ezPathUtils::IsAbsolutePath(sResult))
      return sResult;

    ezStringBuilder sTemp = sResult;
    if (ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sTemp, false))
      return sTemp;

    // only finds a path whose file already exists, but it is what resolves a rooted path or a guid
    sTemp = sResult;
    if (ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTemp))
      return sTemp;

    if (ezToolsProject::IsProjectOpen())
    {
      sTemp = ezToolsProject::GetSingleton()->GetProjectDirectory();
      sTemp.AppendPath(sResult);
      sTemp.MakeCleanPath();
      return sTemp;
    }

    return sResult;
  }

  ezAssetCurator::ezLockedSubAsset ResolveAsset(ezStringView sPathOrGuid)
  {
    ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

    auto asset = pCurator->FindSubAsset(sPathOrGuid, false);

    if (asset.isValid())
      return asset;

    return pCurator->FindSubAsset(sPathOrGuid, true);
  }

  /// Resolves the 'mesh' argument down to a mesh asset guid, or explains why it isn't one.
  bool ResolveMeshArgument(const ezVariantDictionary& arguments, ezMcpToolResult& out_result, ezUuid& out_guid)
  {
    const ezStringView sMesh = ezMcpJson::GetString(arguments, "mesh");

    if (sMesh.IsEmpty())
    {
      out_result.SetError("The 'mesh' argument is required. Pass the guid or path of a mesh asset, as returned by asset_find.");
      return false;
    }

    if (ezAssetCurator::GetSingleton() == nullptr)
    {
      out_result.SetError("No project is open.");
      return false;
    }

    ezUuid guid;
    {
      auto asset = ResolveAsset(sMesh);

      if (!asset.isValid())
      {
        ezStringBuilder sError;
        sError.SetFormat("No asset found for '{}'. Use asset_find to search for it.", sMesh);
        out_result.SetError(sError);
        return false;
      }

      guid = asset->m_Data.m_Guid;
    }

    if (!ezMeshPrefabCreator::IsMeshAsset(guid))
    {
      ezStringBuilder sError;
      sError.SetFormat("'{}' is not a mesh or animated mesh asset, so no prefab can be created from it.", sMesh);
      out_result.SetError(sError);
      return false;
    }

    out_guid = guid;
    return true;
  }

  /// The prefab path that mesh_prefab_info suggests and that mesh_prefab_create uses when the caller
  /// names none: the mesh asset path with the extension swapped. Empty when that file exists, as
  /// creation refuses to overwrite.
  ezStringBuilder SuggestPrefabPath(const ezMeshPrefabSource& source)
  {
    if (source.m_sMeshAssetPath.IsEmpty())
      return {};

    ezStringBuilder sPath = source.m_sMeshAssetPath;
    sPath.ChangeFileExtension("ezPrefab");

    if (ezOSFile::ExistsFile(sPath))
      return {};

    return sPath;
  }

} // namespace

void ezMcpMeshPrefabTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "mesh_prefab_info";
    desc.m_sDescription =
      "Reports what a prefab created from a mesh asset would contain, and which options are usable for it. Modifies nothing. "
      "Returns the render component that fits the mesh (ezLodMeshComponent when the import produced LOD assets, "
      "ezAnimatedMeshComponent for a skinned mesh, ezMeshComponent otherwise), the mesh bounds, any collision mesh asset that "
      "already exists for the same source model, and for every physics mode whether it can be used and why not. "
      "Call this before 'mesh_prefab_create' when the options matter; creating with the defaults needs no info call.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("mesh":{"type":"string","description":"Guid or path of a mesh or animated mesh asset, as returned by asset_find."})"
                          R"(},"required":["mesh"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "mesh_prefab_create";
    desc.m_sDescription =
      "Creates and saves a prefab document that displays one mesh asset: a root game object with the render component that fits the "
      "mesh, and optionally a Jolt collider. This is the same operation as 'Create Prefab...' in the asset browser's context menu. "
      "The collision mesh modes generate an ezJoltCollisionMeshAsset (or convex one) next to the mesh asset, reusing an existing one "
      "built from the same source model rather than creating a duplicate. The box modes need the mesh bounds and therefore a mesh "
      "asset that was transformed at least once. "
      "Fails if the target file already exists - it never overwrites a prefab. The new prefab is not transformed here; call "
      "asset_transform on it before using it in a scene.";
    desc.m_sInputSchema =
      R"({"type":"object","properties":{)"
      R"("mesh":{"type":"string","description":"Guid or path of a mesh or animated mesh asset, as returned by asset_find."},)"
      R"("prefab":{"type":"string","description":"Where to write the prefab. Absolute, or relative to a data directory or its parent. A missing '.ezPrefab' extension is added. Defaults to the mesh asset path with the extension swapped."},)"
      R"("renderComponent":{"type":"string","description":"RTTI name of the component that renders the mesh, e.g. 'ezMeshComponent'. Defaults to what mesh_prefab_info reports as 'renderComponent'."},)"
      R"("physics":{"type":"string","enum":["None","StaticTriangleMesh","StaticConvexHull","StaticBox","DynamicConvexHull","DynamicBox"],"description":"Which collider to set up. Defaults to 'defaultPhysics' from mesh_prefab_info, which is a mode matching an already existing collision mesh asset, or 'None'."},)"
      R"("collisionLayer":{"type":"number","description":"Jolt collision layer for the actor component. Defaults to 0."},)"
      R"("surface":{"type":"string","description":"Guid or path of a surface asset for the collider. Optional."})"
      R"(},"required":["mesh"]})";
  }
}

void ezMcpMeshPrefabTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "mesh_prefab_info")
    ExecuteInfo(arguments, out_result);
  else if (sToolName == "mesh_prefab_create")
    ExecuteCreate(arguments, out_result);
}

void ezMcpMeshPrefabTool::ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezUuid meshGuid;
  if (!ResolveMeshArgument(arguments, out_result, meshGuid))
    return;

  ezMeshPrefabSource source;
  if (ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Failed())
  {
    out_result.SetError("The mesh asset could not be read.");
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableUuid("mesh", source.m_MeshAssetGuid);
  writer.AddVariableString("meshPath", source.m_sMeshAssetPath);
  writer.AddVariableString("sourceModelFile", source.m_sMeshFile);
  writer.AddVariableBool("animated", source.m_bAnimated);
  writer.AddVariableString("renderComponent", source.GetDefaultRenderComponentType());

  writer.BeginArray("lodMeshes");
  for (const ezUuid& lod : source.m_LodGuids)
  {
    ezStringBuilder sGuid;
    ezConversionUtils::ToString(lod, sGuid);
    writer.WriteString(sGuid);
  }
  writer.EndArray();

  writer.AddVariableBool("hasBounds", source.m_bHasBounds);

  if (source.m_bHasBounds)
  {
    writer.BeginObject("bounds");
    writer.AddVariableVec3("center", source.m_vBoundsCenter);
    writer.AddVariableVec3("halfExtents", source.m_vBoundsHalfExtents);
    writer.AddVariableFloat("radius", source.m_fBoundsRadius);
    writer.EndObject();
  }

  writer.AddVariableBool("physicsAvailable", ezMeshPrefabCreator::IsPhysicsAvailable());
  writer.AddVariableString("defaultPhysics", PhysicsToString(source.GetDefaultPhysics()));

  if (source.m_ExistingTriangleColMesh.IsValid())
    writer.AddVariableUuid("existingTriangleCollisionMesh", source.m_ExistingTriangleColMesh);

  if (source.m_ExistingConvexColMesh.IsValid())
    writer.AddVariableUuid("existingConvexCollisionMesh", source.m_ExistingConvexColMesh);

  // Every mode is listed, usable or not, so that a caller can tell a mode that does not exist from
  // one this mesh is missing a prerequisite for.
  writer.BeginArray("physicsModes");
  for (const PhysicsName& n : s_PhysicsNames)
  {
    const ezStringView sReason = GetPhysicsUnavailableReason(source, n.m_Value);

    writer.BeginObject();
    writer.AddVariableString("mode", n.m_sName);
    writer.AddVariableBool("usable", sReason.IsEmpty());

    if (!sReason.IsEmpty())
      writer.AddVariableString("reason", sReason);

    writer.EndObject();
  }
  writer.EndArray();

  const ezStringBuilder sSuggested = SuggestPrefabPath(source);

  if (sSuggested.IsEmpty())
  {
    writer.AddVariableString("note", "A prefab already exists at the default path, so 'prefab' has to be given explicitly. "
                                     "mesh_prefab_create does not overwrite.");
  }
  else
  {
    writer.AddVariableString("suggestedPrefabPath", sSuggested);
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpMeshPrefabTool::ExecuteCreate(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezUuid meshGuid;
  if (!ResolveMeshArgument(arguments, out_result, meshGuid))
    return;

  ezMeshPrefabSource source;
  if (ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Failed())
  {
    out_result.SetError("The mesh asset could not be read.");
    return;
  }

  ezMeshPrefabOptions options;

  {
    const ezStringView sPrefab = ezMcpJson::GetString(arguments, "prefab");

    ezStringBuilder sPath = sPrefab.IsEmpty() ? SuggestPrefabPath(source) : ResolveOutputPath(sPrefab);

    if (sPath.IsEmpty())
    {
      out_result.SetError("A prefab already exists at the default path next to the mesh asset. Pass 'prefab' to write it somewhere "
                          "else - this never overwrites an existing prefab.");
      return;
    }

    // creation resolves the document type from the extension, which a caller often leaves off
    if (!ezPathUtils::HasExtension(sPath, "ezPrefab"))
      sPath.ChangeFileExtension("ezPrefab");

    options.m_sPrefabPath = sPath;
  }

  options.m_sRenderComponentType = ezMcpJson::GetString(arguments, "renderComponent", source.GetDefaultRenderComponentType());

  {
    const ezStringView sPhysics = ezMcpJson::GetString(arguments, "physics");

    if (sPhysics.IsEmpty())
    {
      options.m_Physics = source.GetDefaultPhysics();
    }
    else
    {
      ezMeshPrefabPhysics::Enum mode = ezMeshPrefabPhysics::None;

      if (!PhysicsFromString(sPhysics, mode))
      {
        ezStringBuilder sError;
        sError.SetFormat("'{}' is not a physics mode. Valid values are None, StaticTriangleMesh, StaticConvexHull, StaticBox, "
                         "DynamicConvexHull and DynamicBox.",
          sPhysics);
        out_result.SetError(sError);
        return;
      }

      // checked here, so that the reason names the missing prerequisite
      const ezStringView sReason = GetPhysicsUnavailableReason(source, mode);

      if (!sReason.IsEmpty())
      {
        ezStringBuilder sError;
        sError.SetFormat("Physics mode '{}' cannot be used for this mesh. {}", sPhysics, sReason);
        out_result.SetError(sError);
        return;
      }

      options.m_Physics = mode;
    }
  }

  options.m_uiCollisionLayer = static_cast<ezUInt8>(ezMath::Clamp<ezInt64>(ezMcpJson::GetInt(arguments, "collisionLayer", 0), 0, 31));

  {
    const ezStringView sSurface = ezMcpJson::GetString(arguments, "surface");

    if (!sSurface.IsEmpty())
    {
      auto asset = ResolveAsset(sSurface);

      if (!asset.isValid())
      {
        ezStringBuilder sError;
        sError.SetFormat("No asset found for surface '{}'.", sSurface);
        out_result.SetError(sError);
        return;
      }

      ezStringBuilder sGuid;
      ezConversionUtils::ToString(asset->m_Data.m_Guid, sGuid);
      options.m_sSurfaceAsset = sGuid;
    }
  }

  // not opened: that would put a window in front of the user for something they did not click on
  options.m_bOpenAfterCreate = false;

  const ezStatus res = ezMeshPrefabCreator::CreateMeshPrefab(source, options);

  if (res.Failed())
  {
    out_result.SetError(res.GetMessageString());
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("prefabPath", options.m_sPrefabPath);
  writer.AddVariableUuid("mesh", source.m_MeshAssetGuid);
  writer.AddVariableString("renderComponent", options.m_sRenderComponentType);
  writer.AddVariableString("physics", PhysicsToString(options.m_Physics));
  writer.AddVariableUInt32("lodCount", source.m_LodGuids.GetCount() + 1);

  // the curator has to have seen the file before anything can look it up by guid
  ezAssetCurator::GetSingleton()->CheckFileSystem();

  auto created = ezAssetCurator::GetSingleton()->FindSubAsset(options.m_sPrefabPath, false);

  if (created.isValid())
    writer.AddVariableUuid("prefab", created->m_Data.m_Guid);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
