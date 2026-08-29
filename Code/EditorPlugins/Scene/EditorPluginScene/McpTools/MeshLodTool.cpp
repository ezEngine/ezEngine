#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <EditorPluginScene/McpTools/MeshLodTool.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpMeshLodTool, 1, ezRTTIDefaultAllocator<ezMcpMeshLodTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  ezAssetCurator::ezLockedSubAsset ResolveLodAsset(ezStringView sPathOrGuid)
  {
    ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

    auto asset = pCurator->FindSubAsset(sPathOrGuid, false);

    if (asset.isValid())
      return asset;

    return pCurator->FindSubAsset(sPathOrGuid, true);
  }

  /// Resolves the 'mesh' argument down to a mesh asset guid, or explains why it isn't one.
  bool ResolveLodMeshArgument(const ezVariantDictionary& arguments, ezMcpToolResult& out_result, ezUuid& out_guid)
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
      auto asset = ResolveLodAsset(sMesh);

      if (!asset.isValid())
      {
        ezStringBuilder sError;
        sError.SetFormat("No asset found for '{}'. Use asset_find to search for it.", sMesh);
        out_result.SetError(sError);
        return false;
      }

      guid = asset->m_Data.m_Guid;
    }

    if (!ezMeshLodCreator::IsMeshAsset(guid))
    {
      ezStringBuilder sError;
      sError.SetFormat("'{}' is not a mesh or animated mesh asset, so no LODs can be created for it.", sMesh);
      out_result.SetError(sError);
      return false;
    }

    out_guid = guid;
    return true;
  }

  /// How many LODs to make, clamped to what the creator supports.
  ///
  /// Zero is rejected rather than clamped up: it is far more likely to be a mistake than a request to
  /// do nothing.
  bool ReadLodCount(const ezVariantDictionary& arguments, ezMcpToolResult& out_result, ezUInt32& out_uiCount)
  {
    const ezInt64 iCount = ezMcpJson::GetInt(arguments, "lodCount", 2);

    if (iCount < 1 || iCount > (ezInt64)ezMeshLodCreator::s_uiMaxLods)
    {
      ezStringBuilder sError;
      sError.SetFormat("'lodCount' must be between 1 and {}.", ezMeshLodCreator::s_uiMaxLods);
      out_result.SetError(sError);
      return false;
    }

    out_uiCount = (ezUInt32)iCount;
    return true;
  }

  /// The shared part of both results: what this mesh is and what its LOD ladder looks like.
  void WriteSourceInfo(ezMcpJsonWriter& ref_writer, const ezMeshLodSource& source, ezUInt32 uiLodCount)
  {
    ref_writer.AddVariableUuid("mesh", source.m_MeshAssetGuid);
    ref_writer.AddVariableString("meshPath", source.m_sMeshAssetPath);
    ref_writer.AddVariableString("sourceModelFile", source.m_sMeshFile);
    ref_writer.AddVariableBool("animated", source.m_bAnimated);
    ref_writer.AddVariableString("lodFolder", source.m_sLodFolder);
    ref_writer.AddVariableUInt32("baseSimplification", source.m_uiBaseSimplification);

    ref_writer.BeginArray("lods");
    for (ezUInt32 uiLod = 1; uiLod <= uiLodCount; ++uiLod)
    {
      ref_writer.BeginObject();
      ref_writer.AddVariableUInt32("lod", uiLod);
      ref_writer.AddVariableString("path", ezMeshLodCreator::GetLodPath(source, uiLod));
      ref_writer.AddVariableUInt32("simplification", ezMeshLodCreator::GetLodSimplification(source.m_uiBaseSimplification, uiLod));
      ref_writer.AddVariableUInt32("maxSimplificationError", ezMeshLodCreator::GetLodSimplificationError(uiLod));
      ref_writer.AddVariableBool("exists", source.HasLod(uiLod));
      ref_writer.EndObject();
    }
    ref_writer.EndArray();
  }

} // namespace

void ezMcpMeshLodTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "mesh_lod_info";
    desc.m_sDescription =
      "Reports what LOD mesh assets would be created for a mesh asset, and which of them already exist. Modifies nothing. "
      "Returns the folder the LODs belong in, how much the mesh asset itself is already simplified, and for every level its "
      "path, the simplification percentage (the share of triangles removed) and the tolerated error. "
      "Fails for a procedural primitive, which has no source model to simplify. "
      "Call this before 'mesh_lod_create' when the numbers matter; creating with the defaults needs no info call.";
    desc.m_sInputSchema =
      R"({"type":"object","properties":{)"
      R"("mesh":{"type":"string","description":"Guid or path of a mesh or animated mesh asset, as returned by asset_find."},)"
      R"("lodCount":{"type":"number","description":"How many levels to report, starting at LOD-1. 1 to 4, default 2."})"
      R"(},"required":["mesh"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "mesh_lod_create";
    desc.m_sDescription =
      "Creates and saves the LOD mesh assets that sit next to a mesh asset, as 'Create LODs...' in the asset browser's context "
      "menu does. Each LOD is the same source model imported again with stronger mesh simplification, sharing the mesh asset's "
      "import settings and materials. They are written to '<MeshName>_data/LOD-1.ezMeshAsset' and so on, which is where "
      "'mesh_prefab_create' looks for them: a prefab created afterwards then uses an ezLodMeshComponent instead of an "
      "ezMeshComponent. "
      "Existing LOD assets are left alone unless 'overwrite' is set, and an overwritten one keeps its guid so that anything "
      "referencing it keeps working. "
      "The new LOD assets are not transformed here; call asset_transform on each of them, and on the prefab, before using them.";
    desc.m_sInputSchema =
      R"({"type":"object","properties":{)"
      R"("mesh":{"type":"string","description":"Guid or path of a mesh or animated mesh asset, as returned by asset_find."},)"
      R"("lodCount":{"type":"number","description":"How many LOD assets to create, starting at LOD-1. LOD 0 is the mesh asset itself, which is never touched. 1 to 4, default 2."},)"
      R"("overwrite":{"type":"boolean","description":"Rewrite LOD assets that already exist. Default false, which skips them, because an existing LOD may have been tuned by hand."})"
      R"(},"required":["mesh"]})";
  }
}

void ezMcpMeshLodTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "mesh_lod_info")
    ExecuteInfo(arguments, out_result);
  else if (sToolName == "mesh_lod_create")
    ExecuteCreate(arguments, out_result);
}

void ezMcpMeshLodTool::ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezUuid meshGuid;
  if (!ResolveLodMeshArgument(arguments, out_result, meshGuid))
    return;

  ezUInt32 uiLodCount = 0;
  if (!ReadLodCount(arguments, out_result, uiLodCount))
    return;

  ezMeshLodSource source;
  if (ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Failed())
  {
    out_result.SetError("The mesh asset could not be read.");
    return;
  }

  if (source.m_bIsPrimitive)
  {
    out_result.SetError("This mesh asset uses a procedural primitive, not a model file, so no LODs can be generated from it.");
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteSourceInfo(writer, source, uiLodCount);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpMeshLodTool::ExecuteCreate(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezUuid meshGuid;
  if (!ResolveLodMeshArgument(arguments, out_result, meshGuid))
    return;

  ezMeshLodOptions options;
  if (!ReadLodCount(arguments, out_result, options.m_uiLodCount))
    return;

  options.m_bOverwriteExisting = ezMcpJson::GetBool(arguments, "overwrite", false);

  ezMeshLodSource source;
  if (ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Failed())
  {
    out_result.SetError("The mesh asset could not be read.");
    return;
  }

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  const ezStatus res = ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped);

  if (res.Failed())
  {
    out_result.SetError(res.GetMessageString());
    return;
  }

  // Re-read, so that the reported guids and 'exists' flags describe the state after the run rather
  // than the one it started from.
  ezMeshLodSource after;
  if (ezMeshLodCreator::GatherMeshLodSource(meshGuid, after).Failed())
    after = source;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableUInt32("created", uiCreated);
  writer.AddVariableUInt32("skipped", uiSkipped);

  if (uiSkipped > 0 && !options.m_bOverwriteExisting)
  {
    writer.AddVariableString("note", "Skipped LODs already existed. Pass 'overwrite' to rewrite them.");
  }

  WriteSourceInfo(writer, after, options.m_uiLodCount);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
