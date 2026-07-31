#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpTranslation.h>
#include <EditorPluginMcp/McpTools/AssetTool.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpAssetTool, 1, ezRTTIDefaultAllocator<ezMcpAssetTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// The largest number of assets asset_find returns. A real project has thousands, and an unfiltered
  /// listing would fill the client's context with paths it did not ask for. The total is reported
  /// alongside, so the agent can tell 'that is all of them' from 'narrow your filter'.
  constexpr ezUInt32 s_uiMaxAssetResults = 200;

  /// The names of ezAssetInfo::TransformState, in enum order.
  ///
  /// These are the strings the tools report and accept as a filter, so they are part of the interface
  /// an agent sees, not just a debug aid.
  constexpr const char* s_szTransformStateNames[ezAssetInfo::TransformState::COUNT] = {
    "Unknown",
    "UpToDate",
    "NeedsImport",
    "NeedsTransform",
    "NeedsThumbnail",
    "TransformError",
    "MissingTransformDependency",
    "MissingThumbnailDependency",
    "MissingPackageDependency",
    "CircularDependency",
  };

  /// Deliberately the same severity names that log_read reports, so an agent reading a transform log
  /// and the editor log does not have to learn two vocabularies. Kept as a separate copy rather than
  /// shared with LogTool, because exporting it would couple two otherwise unrelated tool providers
  /// through a header for seven strings. If a third tool needs these, move them into a common place.
  ezStringView AssetLogSeverityToString(ezLogMsgType::Enum type)
  {
    switch (type)
    {
      case ezLogMsgType::ErrorMsg: return "error";
      case ezLogMsgType::SeriousWarningMsg: return "serious-warning";
      case ezLogMsgType::WarningMsg: return "warning";
      case ezLogMsgType::SuccessMsg: return "success";
      case ezLogMsgType::InfoMsg: return "info";
      case ezLogMsgType::DevMsg: return "dev";
      case ezLogMsgType::DebugMsg: return "debug";
      default: return "other";
    }
  }

  ezStringView TransformStateToString(ezAssetInfo::TransformState state)
  {
    if (state < 0 || state >= ezAssetInfo::TransformState::COUNT)
      return "Unknown";

    return s_szTransformStateNames[state];
  }

  /// Returns COUNT if the name matches no state, which the caller has to report as a bad argument
  /// rather than silently filtering on something else.
  ezAssetInfo::TransformState TransformStateFromString(ezStringView sName)
  {
    for (ezUInt32 i = 0; i < ezAssetInfo::TransformState::COUNT; ++i)
    {
      if (sName.IsEqual_NoCase(s_szTransformStateNames[i]))
        return static_cast<ezAssetInfo::TransformState>(i);
    }

    return ezAssetInfo::TransformState::COUNT;
  }

  /// Wraps a semicolon separated list in semicolons, which is the form the filters compare against.
  ///
  /// The type filter of ezAssetBrowserAttribute is already stored this way (";Texture 2D;Texture 3D;"),
  /// so an agent forwarding what rtti_type_properties reported would pass the delimiters along. Both
  /// forms have to work, because requiring the agent to strip delimiters it did not add is a trap.
  void NormalizeDelimitedList(ezStringBuilder& ref_sList)
  {
    ref_sList.Trim(" ");

    if (ref_sList.IsEmpty())
      return;

    if (!ref_sList.StartsWith(";"))
      ref_sList.Prepend(";");

    if (!ref_sList.EndsWith(";"))
      ref_sList.Append(";");
  }

  /// Extracts the guid of a 'ref:<guid>' / 'ref-all:<guid>' prefix out of a search text.
  ///
  /// This is the syntax the asset browser's search box uses for a reverse lookup. It is supported here
  /// so a search a user typed into the browser can be pasted through unchanged, but the documented way
  /// is the 'usedBy' argument - a schema an agent can read beats a prefix it has to be told about.
  ///
  /// Returns false if the text carries no such prefix, in which case the outputs are untouched.
  bool ExtractUsesSearch(ezStringView sText, ezUuid& out_guid, bool& out_bTransitive)
  {
    ezStringBuilder sTemp = sText;

    const char* szRefAll = ezStringUtils::FindSubString_NoCase(sTemp, "ref-all:");
    const char* szRef = ezStringUtils::FindSubString_NoCase(sTemp, "ref:");

    if (szRefAll == nullptr && szRef == nullptr)
      return false;

    const bool bTransitive = szRefAll != nullptr;
    const char* szGuid = bTransitive ? szRefAll + ezStringUtils::GetStringElementCount("ref-all:") : szRef + ezStringUtils::GetStringElementCount("ref:");

    if (!ezConversionUtils::IsStringUuid(szGuid))
      return false;

    out_guid = ezConversionUtils::ConvertStringToUuid(szGuid);
    out_bTransitive = bTransitive;
    return true;
  }

  /// Resolves a guid or a path to an asset.
  ///
  /// FindSubAsset()'s fast path handles guids and absolute paths, but *not* the data dir parent
  /// relative path ("Testing Chambers/Prefabs/Barrel.ezMeshAsset") - which is exactly the form
  /// asset_find reports as 'path'. Without the exhaustive fallback the tools do not compose: feeding
  /// asset_find's own output back into asset_info fails. The fallback is a linear scan over all known
  /// assets, but it only runs once the cheap lookup has already missed, and it is the same order of
  /// work asset_find does anyway.
  ezAssetCurator::ezLockedSubAsset ResolveAsset(ezStringView sPathOrGuid)
  {
    ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

    auto asset = pCurator->FindSubAsset(sPathOrGuid, false);

    if (asset.isValid())
      return asset;

    return pCurator->FindSubAsset(sPathOrGuid, true);
  }

  /// Collects the assets that the given asset references.
  ///
  /// The transitive case is exactly the dependency hull. The direct case has no curator call that
  /// returns guids, so it is the hull filtered down to the entries that appear in the asset's own
  /// dependency sets. Those sets hold "a data dir relative path or a GUID" (see ezAssetDocumentInfo),
  /// so each candidate is looked for under its guid and under both of its relative path spellings.
  void CollectReferencedAssets(const ezSubAsset& subAsset, bool bTransitive, ezSet<ezUuid>& out_deps)
  {
    ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

    pCurator->GenerateTransitiveAssetHull(subAsset.m_Data.m_Guid, out_deps,
      ezDependencyFlags::Transform | ezDependencyFlags::Thumbnail | ezDependencyFlags::Package);

    // the hull includes the asset itself, which is never an answer to 'what does this use'
    out_deps.Remove(subAsset.m_Data.m_Guid);

    if (bTransitive)
      return;

    const ezAssetDocumentInfo* pInfo = subAsset.m_pAssetInfo != nullptr ? subAsset.m_pAssetInfo->m_Info.Borrow() : nullptr;

    if (pInfo == nullptr)
    {
      // without the asset's own lists there is nothing to filter against, and reporting the whole
      // transitive hull as if it were direct would be a wrong answer rather than a partial one
      out_deps.Clear();
      return;
    }

    ezSet<ezUuid> direct;
    ezStringBuilder sGuid;

    auto IsListed = [pInfo](ezStringView sKey) -> bool
    {
      return pInfo->m_TransformDependencies.Contains(sKey) || pInfo->m_ThumbnailDependencies.Contains(sKey) ||
             pInfo->m_PackageDependencies.Contains(sKey);
    };

    for (const ezUuid& guid : out_deps)
    {
      // Deliberately no check that the curator knows this guid: a reference to a missing asset is
      // precisely what a caller asking about references wants to see.
      ezConversionUtils::ToString(guid, sGuid);

      if (IsListed(sGuid))
      {
        direct.Insert(guid);
        continue;
      }

      // The same reference may be stored as a path instead, in either relative spelling. Both are
      // asked for, because which one a document wrote depends on how the reference was authored, and a
      // path-stored reference that is not recognised here is reported as indirect - which is wrong,
      // not merely incomplete.
      const ezAssetCurator::ezLockedSubAsset dep = pCurator->GetSubAsset(guid);

      if (dep.isValid() && dep->m_pAssetInfo != nullptr && dep->m_pAssetInfo->m_Path.IsValid())
      {
        const ezDataDirPath& path = dep->m_pAssetInfo->m_Path;

        if (IsListed(path.GetDataDirRelativePath()) || IsListed(path.GetDataDirParentRelativePath()))
        {
          direct.Insert(guid);
        }
      }
    }

    out_deps.Swap(direct);
  }

  /// Writes the state of the background asset processor as a nested object.
  ///
  /// 'Stopped' and 'paused' are different things: the first means the processor was switched off, the
  /// second that it is on but temporarily held back (asset import does this). Both stop progress, and a
  /// caller watching the transform state counts has to be able to tell why nothing is moving.
  void WriteProcessorStatus(ezMcpJsonWriter& ref_writer, ezStringView sName)
  {
    ezAssetProcessor* pProcessor = ezAssetProcessor::GetSingleton();

    if (pProcessor == nullptr)
      return;

    ref_writer.BeginObject(sName);

    switch (pProcessor->GetProcessorState())
    {
      case ezAssetProcessor::ProcessorState::Stopped:
        ref_writer.AddVariableString("state", "Stopped");
        break;
      case ezAssetProcessor::ProcessorState::Running:
        ref_writer.AddVariableString("state", "Running");
        break;
      case ezAssetProcessor::ProcessorState::Stopping:
        ref_writer.AddVariableString("state", "Stopping");
        break;
    }

    const ezUInt32 uiProcessCount = pProcessor->GetProcessCount();

    ezUInt32 uiBusy = 0;
    ezUInt32 uiCrashed = 0;

    for (ezUInt32 i = 0; i < uiProcessCount; ++i)
    {
      const ezEditorProcessorState state = pProcessor->GetProcessState(i);

      if (state.m_bCrashed)
        ++uiCrashed;
      else if (state.m_bRunning)
        ++uiBusy;
    }

    ref_writer.AddVariableUInt32("processes", uiProcessCount);
    ref_writer.AddVariableUInt32("transformingNow", uiBusy);

    // Refcounted and set by whatever is currently importing, so it can be non-zero even while the
    // processor is Running. Nothing moves while it is set.
    if (pProcessor->m_iPauseProcessing > 0)
    {
      ref_writer.AddVariableBool("paused", true);
      ref_writer.AddVariableString("pausedReason", "Something in the editor is temporarily holding off background processing, usually an asset import. This clears on its own.");
    }

    // a crashed processor stops making progress silently, so it is always worth reporting
    if (uiCrashed > 0)
      ref_writer.AddVariableUInt32("crashedProcesses", uiCrashed);

    ref_writer.EndObject();
  }

  /// Writes a set of strings as an array, but only if it has any content.
  ///
  /// Empty arrays repeated across every asset are pure token cost. This does mean a client cannot tell
  /// 'no dependencies' from 'not reported', which is the trade Status.md settles on for optional detail.
  void WriteStringSet(ezMcpJsonWriter& ref_writer, ezStringView sName, const ezSet<ezString>& values)
  {
    if (values.IsEmpty())
      return;

    ref_writer.BeginArray(sName);
    for (const ezString& sValue : values)
    {
      ref_writer.WriteString(sValue);
    }
    ref_writer.EndArray();
  }
} // namespace

void ezMcpAssetTool::OnActivate()
{
  if (m_ProgressSubscription != 0)
    return;

  // The processor is a singleton that only exists while a project is open, which is also when this
  // provider is activated.
  if (ezAssetProcessor* pProcessor = ezAssetProcessor::GetSingleton())
  {
    m_ProgressSubscription = pProcessor->m_ProgressEvents.AddEventHandler(ezMakeDelegate(&ezMcpAssetTool::ProcessorProgressEventHandler, this));
  }
}

void ezMcpAssetTool::OnDeactivate()
{
  if (m_ProgressSubscription == 0)
    return;

  if (ezAssetProcessor* pProcessor = ezAssetProcessor::GetSingleton())
  {
    // takes the id by reference and resets it
    pProcessor->m_ProgressEvents.RemoveEventHandler(m_ProgressSubscription);
  }

  m_ProgressSubscription = 0;
}

void ezMcpAssetTool::ProcessorProgressEventHandler(const ezAssetProcessorProgressEvent& e)
{
  // Only finished work is recorded. A 'started' entry would be superseded moments later and the
  // in-flight count is already reported by asset_health.
  if (e.m_Type != ezAssetProcessorProgressEvent::Type::ProcessingFinished)
    return;

  // The path is stored exactly as the event delivers it - GetDataDirRelativePath(), which omits the
  // data directory and so is shorter than the path the other tools report. Resolving it through the
  // curator would be more consistent, but this runs on a processor worker thread and GetSubAsset()
  // takes the curator lock, which the transform path already holds in places. The existing UI handler
  // avoids the same trap by keeping only the guid and resolving on the main thread. The guid is
  // returned alongside and is unambiguous, so the shorter path is the cheaper trade.
  RecordHistory(e.m_AssetGuid, e.m_sAssetPath, e.m_Result, e.m_EndTime - e.m_TransformStartTime, true);
}

void ezMcpAssetTool::RecordHistory(const ezUuid& guid, ezStringView sPath, const ezTransformStatus& status, ezTime duration, bool bBackground)
{
  // Called from the processor's worker threads as well as from the main thread via asset_transform.
  EZ_LOCK(m_HistoryMutex);

  if (m_History.GetCount() >= s_uiMaxHistoryEntries)
  {
    m_History.PopFront();
  }

  HistoryEntry& entry = m_History.ExpandAndGetRef();
  entry.m_uiId = m_uiNextHistoryId++;
  entry.m_Guid = guid;
  entry.m_sPath = sPath;
  entry.m_sMessage = status.m_sMessage;
  entry.m_Result = status.m_Result;
  entry.m_Duration = duration;
  entry.m_EndTime = ezTime::Now();
  entry.m_bBackground = bBackground;
}

void ezMcpAssetTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_types";
    desc.m_sDescription = "Lists every document/asset type the editor knows, with its file extension, whether it can be created, "
                          "its asset browser category, which asset slots it is compatible with, and a link to its online "
                          "documentation where one exists. Use this to find the correct type name and extension before creating "
                          "or looking for an asset, and the documentation link to find out what a type is actually for.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("extension":{"type":"string","description":"Only return types whose file extension contains this text, e.g. 'Mesh'."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_find";
    desc.m_sDescription = "Searches the assets of the project and returns the GUID, path and type of each match. This is the entry "
                          "point into the asset database: narrow down to a few GUIDs here, then use asset_info, asset_uses or "
                          "asset_thumbnail on those. All filters combine with AND. Results are capped, but the total number of "
                          "matches is reported. Note that this searches the editor's asset database, not the file system - it "
                          "knows about GUIDs, types and references, which the file tools cannot answer.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"Matches against the asset name, its path and its GUID, case insensitive. Multiple words separated by spaces must all match, in any order; a word prefixed with '-' must not match. E.g. 'stone -rough'. A partial GUID works too."},)"
                          R"("path":{"type":"string","description":"Only return assets whose data directory relative path starts with this folder, e.g. 'Textures/Nature'."},)"
                          R"("type":{"type":"string","description":"Only return assets of these document types, separated by semicolons, e.g. 'Texture 2D;Texture 3D'. Use asset_types for the valid names. This accepts the value of an ezAssetBrowserAttribute's TypeFilter unchanged."},)"
                          R"("tag":{"type":"string","description":"Only return assets carrying this asset document tag. Tags restrict which assets may go into a specific property - the required tag is reported by the ezAssetBrowserAttribute on that property, see rtti_type_properties."},)"
                          R"("usedBy":{"type":"string","description":"Reverse lookup: only return assets that are directly referenced by this asset (GUID or path). Answers 'what does this asset use'."},)"
                          R"("uses":{"type":"string","description":"Reverse lookup: only return assets that reference this asset (GUID or path). Answers 'who uses this'. Combine with 'transitive' for indirect uses."},)"
                          R"("transitive":{"type":"boolean","description":"If true, 'uses' and 'usedBy' also follow indirect references. Default false."},)"
                          R"("transformState":{"type":"string","description":"Only return assets in this transform state. One of: Unknown, UpToDate, NeedsImport, NeedsTransform, NeedsThumbnail, TransformError, MissingTransformDependency, MissingThumbnailDependency, MissingPackageDependency, CircularDependency."},)"
                          R"("subAssets":{"type":"boolean","description":"If true, also return sub-assets, e.g. the individual animation clips inside an animation asset. Default false, which returns only main assets."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_info";
    desc.m_sDescription = "Returns the details of one asset: its absolute and relative paths, type, transform state, tags, "
                          "sub-assets, its dependencies, a link to the documentation of its type, and - if the last transform "
                          "failed - the log messages explaining why. "
                          "The log messages are the reason to call this on a broken asset instead of searching the editor log.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("asset":{"type":"string","description":"The asset GUID or its path, as returned by asset_find. Either form works."},)"
                          R"("dependencies":{"type":"boolean","description":"If true, include the transform, thumbnail and package dependency lists. Default true. Set to false on assets with many references to keep the result small."})"
                          R"(},"required":["asset"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_uses";
    desc.m_sDescription = "Returns what an asset references and what references it. This is the question that is genuinely hard to "
                          "answer by reading files, because references are stored as GUIDs. Use it before deleting or changing an "
                          "asset, to find out what would break.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("asset":{"type":"string","description":"The asset GUID or its path."},)"
                          R"("direction":{"type":"string","description":"'uses' returns what references this asset, 'usedBy' returns what this asset references, 'both' returns both. Default 'both'."},)"
                          R"("transitive":{"type":"boolean","description":"If true, also follow indirect references. Default false. This can return a lot on a heavily shared asset such as a common material."})"
                          R"(},"required":["asset"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_thumbnail";
    desc.m_sDescription = "Returns the absolute path of an asset's thumbnail image (a .jpg), plus whether that file currently "
                          "exists and whether it is up to date. The image data is not returned - read the file if you need it. "
                          "Not every asset type has a thumbnail, and the file only exists after the asset was transformed.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("asset":{"type":"string","description":"The asset GUID or its path."})"
                          R"(},"required":["asset"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_transform";
    desc.m_sDescription = "Transforms one asset, i.e. compiles it into its runtime format, and returns whether that succeeded. "
                          "On failure the returned message says why, and asset_info returns the full log. "
                          "IMPORTANT: this blocks the editor until it finishes, which for a large asset such as a scene can take "
                          "minutes - call it with a generous timeout. Only one asset at a time is supported on purpose; there is "
                          "no tool to transform everything, because that would block the editor for far too long.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("asset":{"type":"string","description":"The asset GUID or its path."},)"
                          R"("force":{"type":"boolean","description":"If true, transform even if the asset is already up to date. Default false. Use this when the output looks wrong but the editor thinks nothing changed."})"
                          R"(},"required":["asset"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_health";
    desc.m_sDescription = "Returns how many assets are in each transform state, plus whether the background asset processor is "
                          "running and how many assets it is transforming right now. Cheap enough to call routinely, e.g. after "
                          "changing assets, to check whether anything broke. Note that while the processor is Running the counts "
                          "are moving on their own, so a large NeedsTransform count means 'not got to yet' rather than 'broken'. "
                          "If it reports assets in a failed state, use asset_find with that transformState to get the list, then "
                          "asset_info for the reason.";
    desc.m_sInputSchema = R"({"type":"object","properties":{}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_history";
    desc.m_sDescription = "Returns the assets that were transformed recently, newest last, with how long each took and whether it "
                          "succeeded. Use this after changing something to see what the editor rebuilt in response, which the "
                          "transform state counts cannot tell you. Structured, so unlike log_read it cannot be drowned out by "
                          "unrelated editor output. Only covers this editor session, and only since a project was opened. "
                          "The 'path' of an entry may be shorter than the path the other asset tools report (it omits the data "
                          "directory), so use the GUID to identify an asset unambiguously.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("count":{"type":"number","description":"How many of the most recent entries to return. Default 50."},)"
                          R"("sinceId":{"type":"number","description":"Only return entries with an id greater than this. Pass the 'lastId' of a previous call to get just what happened since - better than guessing a count."},)"
                          R"("failedOnly":{"type":"boolean","description":"If true, only return transforms that did not succeed. Default false."},)"
                          R"("asset":{"type":"string","description":"Only return entries for this asset, given as a GUID or a path substring."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_processor";
    desc.m_sDescription = "Starts or stops the background asset processor, which transforms outdated assets on its own in "
                          "separate processes. Use this when asset_health reports it as Stopped and assets are not becoming "
                          "UpToDate, or to stop it so it does not compete for CPU with something else. Unlike asset_transform "
                          "this returns immediately - it only flips the switch, the work happens in the background afterwards. "
                          "Call with no arguments to just query the current state.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("action":{"type":"string","description":"'start' begins background processing, 'stop' ends it after the assets currently in flight finish, 'forceStop' kills the running processes immediately, 'query' only reports the state and changes nothing. Default 'query'."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_importers";
    desc.m_sDescription =
      "Lists the source file types that can be imported as assets - fbx, gltf, png and so on - and the import modes available for "
      "each. Call this before 'asset_import': the mode name is what 'asset_import' takes, and it decides how the file is "
      "interpreted, e.g. whether a texture is treated as colour data or as a normal map. Pass a file to see only the modes that "
      "apply to it.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("file":{"type":"string","description":"A source file, or just an extension like 'png'. Only report importers that accept it, with the modes they offer for it."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "asset_import";
    desc.m_sDescription =
      "Imports a source file - a mesh, texture, animation and so on - as an asset document, the same way the asset browser's import "
      "does. The new document is created next to the source file, with the asset extension replacing the original one, and is saved "
      "immediately. "
      "It is NOT opened; use 'document_open' if you want to adjust its properties afterwards, which is usually the next step - the "
      "import only sets what it can infer from the file. "
      "If an asset for that source file already exists this reports 'alreadyImported' and changes nothing, rather than overwriting "
      "work. Call 'asset_importers' first to find the mode to use.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("file":{"type":"string","description":"The source file to import. Absolute, or relative to the data directory parent. Must be inside the project."},)"
                          R"("mode":{"type":"string","description":"How to interpret the file, e.g. 'TextureImport.Diffuse'. See 'asset_importers'. If omitted the highest priority mode for the file is used, which is what the import dialog preselects."})"
                          R"(},"required":["file"]})";
  }
}

void ezMcpAssetTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "asset_types")
    ExecuteListTypes(arguments, out_result);
  else if (sToolName == "asset_find")
    ExecuteFind(arguments, out_result);
  else if (sToolName == "asset_info")
    ExecuteInfo(arguments, out_result);
  else if (sToolName == "asset_uses")
    ExecuteUses(arguments, out_result);
  else if (sToolName == "asset_thumbnail")
    ExecuteThumbnail(arguments, out_result);
  else if (sToolName == "asset_transform")
    ExecuteTransform(arguments, out_result);
  else if (sToolName == "asset_health")
    ExecuteHealth(arguments, out_result);
  else if (sToolName == "asset_processor")
    ExecuteProcessor(arguments, out_result);
  else if (sToolName == "asset_history")
    ExecuteHistory(arguments, out_result);
  else if (sToolName == "asset_importers")
    ExecuteListImporters(arguments, out_result);
  else if (sToolName == "asset_import")
    ExecuteImport(arguments, out_result);
}

void ezMcpAssetTool::ExecuteHistory(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezInt64 iCount = ezMath::Clamp<ezInt64>(ezMcpJson::GetInt(arguments, "count", 50), 1, s_uiMaxHistoryEntries);
  const ezUInt64 uiSinceId = static_cast<ezUInt64>(ezMath::Max<ezInt64>(0, ezMcpJson::GetInt(arguments, "sinceId", 0)));
  const bool bFailedOnly = ezMcpJson::GetBool(arguments, "failedOnly", false);
  const ezStringView sAssetFilter = ezMcpJson::GetString(arguments, "asset");

  // Resolve the asset filter once, so that a guid, an exact path and a path substring all work. An
  // unresolvable value is not an error - it is still usable as a substring match against the paths.
  ezUuid filterGuid;
  if (!sAssetFilter.IsEmpty())
  {
    auto asset = ResolveAsset(sAssetFilter);

    if (asset.isValid())
      filterGuid = asset->m_Data.m_Guid;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezUInt32 uiTotalMatches = 0;
  ezUInt64 uiLastId = 0;

  {
    EZ_LOCK(m_HistoryMutex);

    // Walk backwards to find where the newest 'count' matches begin, so the output can still be
    // written oldest-first without collecting into a temporary array.
    ezHybridArray<const HistoryEntry*, 64> selected;

    for (ezUInt32 i = m_History.GetCount(); i > 0; --i)
    {
      const HistoryEntry& entry = m_History[i - 1];

      if (entry.m_uiId <= uiSinceId)
        break; // ids increase with position, so everything before this is older still

      if (bFailedOnly && entry.m_Result == ezTransformResult::Success)
        continue;

      if (!sAssetFilter.IsEmpty())
      {
        const bool bGuidMatches = filterGuid.IsValid() && entry.m_Guid == filterGuid;
        const bool bPathMatches = entry.m_sPath.FindSubString_NoCase(sAssetFilter) != nullptr;

        if (!bGuidMatches && !bPathMatches)
          continue;
      }

      ++uiTotalMatches;

      if (selected.GetCount() < static_cast<ezUInt32>(iCount))
        selected.PushBack(&entry);
    }

    writer.BeginArray("transforms");

    // selected is newest-first, so it is written in reverse to end up chronological
    for (ezUInt32 i = selected.GetCount(); i > 0; --i)
    {
      const HistoryEntry& entry = *selected[i - 1];

      writer.BeginObject();
      writer.AddVariableUInt64("id", entry.m_uiId);
      writer.AddVariableUuid("guid", entry.m_Guid);
      writer.AddVariableString("path", entry.m_sPath);

      switch (entry.m_Result)
      {
        case ezTransformResult::Success:
          writer.AddVariableString("result", "Success");
          break;
        case ezTransformResult::Failure:
          writer.AddVariableString("result", "Failure");
          break;
        case ezTransformResult::NeedsImport:
          writer.AddVariableString("result", "NeedsImport");
          break;
      }

      // rounded to milliseconds - the sub-millisecond digits are noise and cost tokens
      writer.AddVariableInt64("durationMs", static_cast<ezInt64>(entry.m_Duration.GetMilliseconds()));

      // how long ago this finished, which is more useful to a model than an absolute timestamp it has
      // no reference point for
      writer.AddVariableInt64("secondsAgo", static_cast<ezInt64>((ezTime::Now() - entry.m_EndTime).GetSeconds()));

      if (!entry.m_sMessage.IsEmpty())
        writer.AddVariableString("message", entry.m_sMessage);

      // background is the normal case; only the exception is worth the tokens
      if (!entry.m_bBackground)
        writer.AddVariableString("triggeredBy", "asset_transform");

      writer.EndObject();

      uiLastId = ezMath::Max(uiLastId, entry.m_uiId);
    }

    writer.EndArray();

    writer.AddVariableUInt32("totalMatches", uiTotalMatches);
    writer.AddVariableUInt32("returned", selected.GetCount());
    writer.AddVariableBool("truncated", uiTotalMatches > selected.GetCount());
  }

  // What to pass as 'sinceId' next time. Zero when nothing matched, in which case the caller should
  // keep whatever it had rather than resetting to the start of the buffer.
  if (uiLastId > 0)
    writer.AddVariableUInt64("lastId", uiLastId);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::WriteAssetIdentity(ezMcpJsonWriter& ref_writer, const ezSubAsset& subAsset)
{
  ref_writer.AddVariableUuid("guid", subAsset.m_Data.m_Guid);
  ref_writer.AddVariableString("name", subAsset.GetName());

  ezStringBuilder sIdentifier;
  subAsset.GetSubAssetIdentifier(sIdentifier);
  ref_writer.AddVariableString("path", sIdentifier);

  ref_writer.AddVariableString("type", subAsset.m_Data.m_sSubAssetsDocumentTypeName);

  if (!subAsset.m_bMainAsset)
    ref_writer.AddVariableBool("isSubAsset", true);
}

void ezMcpAssetTool::ExecuteListTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sFilter = ezMcpJson::GetString(arguments, "extension");

  ezMcpJsonWriter writer;
  writer.BeginArray();

  // this map is ordered by type name, so the output is stable
  for (auto it : ezDocumentManager::GetAllDocumentDescriptors())
  {
    const ezDocumentTypeDescriptor* pDesc = it.Value();

    if (!sFilter.IsEmpty() && pDesc->m_sFileExtension.FindSubString_NoCase(sFilter) == nullptr)
      continue;

    writer.BeginObject();

    writer.AddVariableString("typeName", pDesc->m_sDocumentTypeName);

    // What the user sees in the asset browser, which is not the type name. Without this an agent
    // cannot connect what a user describes ('the Decal asset') to the name every other tool wants.
    ezMcpTranslation::AddOptionalString(writer, "displayName", ezMcpTranslation::GetDisplayName(pDesc->m_sDocumentTypeName));

    writer.AddVariableString("extension", pDesc->m_sFileExtension);
    writer.AddVariableBool("canCreate", pDesc->m_bCanCreate);
    writer.AddVariableString("category", pDesc->m_sAssetCategory);

    // the RTTI name of the document class - the entry point for finding the code that handles this type
    writer.AddVariableString("documentClass", pDesc->m_pDocumentType != nullptr ? pDesc->m_pDocumentType->GetTypeName() : ezStringView());

    // The link to the online documentation for this asset type, which is prose explaining what the type
    // is for - something no amount of reflection data conveys.
    ezMcpTranslation::AddOptionalString(writer, "helpUrl", ezMcpTranslation::GetHelpURL(pDesc->m_sDocumentTypeName));

    writer.BeginArray("compatibleTypes");
    for (const ezString& sCompatible : pDesc->m_CompatibleTypes)
    {
      writer.WriteString(sCompatible);
    }
    writer.EndArray();

    writer.EndObject();
  }

  writer.EndArray();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteFind(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("No project is open, so there is no asset database to search.");
    return;
  }

  const ezStringView sNameFilter = ezMcpJson::GetString(arguments, "name");
  const ezStringView sTransformState = ezMcpJson::GetString(arguments, "transformState");
  const bool bIncludeSubAssets = ezMcpJson::GetBool(arguments, "subAssets", false);
  bool bTransitive = ezMcpJson::GetBool(arguments, "transitive", false);

  ezStringBuilder sPathFilter = ezMcpJson::GetString(arguments, "path");
  sPathFilter.MakeCleanPath();
  // matching a folder prefix only makes sense with the trailing separator, otherwise 'Text' would also
  // match the folder 'Textures'
  if (!sPathFilter.IsEmpty() && !sPathFilter.EndsWith("/"))
    sPathFilter.Append("/");

  ezStringBuilder sTypeFilter = ezMcpJson::GetString(arguments, "type");
  NormalizeDelimitedList(sTypeFilter);

  ezStringBuilder sTagFilter = ezMcpJson::GetString(arguments, "tag");
  sTagFilter.Trim(" ");

  ezAssetInfo::TransformState stateFilter = ezAssetInfo::TransformState::COUNT;
  if (!sTransformState.IsEmpty())
  {
    stateFilter = TransformStateFromString(sTransformState);

    if (stateFilter == ezAssetInfo::TransformState::COUNT)
    {
      ezStringBuilder sError;
      sError.SetFormat("'{}' is not a valid transform state. Valid values are: ", sTransformState);
      for (ezUInt32 i = 0; i < ezAssetInfo::TransformState::COUNT; ++i)
      {
        if (i > 0)
          sError.Append(", ");
        sError.Append(s_szTransformStateNames[i]);
      }
      out_result.SetError(sError);
      return;
    }
  }

  // The reverse lookups produce a set of GUIDs that the result is restricted to. An empty set is not
  // the same as 'no restriction', so the flag has to be tracked separately - an asset that nothing
  // references must return zero results, not everything.
  bool bRestrictToSet = false;
  ezSet<ezUuid> allowedGuids;

  ezSearchPatternFilter searchFilter;
  {
    // 'ref:<guid>' inside the name filter is the asset browser's syntax for a reverse lookup. It is
    // accepted so a search copied out of the browser works, but 'uses' is the documented argument.
    ezUuid refGuid;
    bool bRefTransitive = false;

    if (!sNameFilter.IsEmpty() && ExtractUsesSearch(sNameFilter, refGuid, bRefTransitive))
    {
      bRestrictToSet = true;
      pCurator->FindAllUses(refGuid, allowedGuids, bRefTransitive);
    }
    else
    {
      ezStringBuilder sCleanName = sNameFilter;
      sCleanName.MakeCleanPath();
      sCleanName.ReplaceAll("*", "");
      searchFilter.SetSearchText(sCleanName);
    }
  }

  // 'uses' asks who references the given asset, which is exactly FindAllUses().
  const ezStringView sUses = ezMcpJson::GetString(arguments, "uses");
  if (!sUses.IsEmpty())
  {
    auto asset = ResolveAsset(sUses);

    if (!asset.isValid())
    {
      ezStringBuilder sError;
      sError.SetFormat("No asset found for 'uses' filter '{}'. Pass a GUID or a path as returned by asset_find.", sUses);
      out_result.SetError(sError);
      return;
    }

    bRestrictToSet = true;
    pCurator->FindAllUses(asset->m_Data.m_Guid, allowedGuids, bTransitive);
  }

  // 'usedBy' asks the opposite - what the given asset references. There is no direct call for the
  // transitive case that returns GUIDs, so the dependency lists are resolved by hand.
  const ezStringView sUsedBy = ezMcpJson::GetString(arguments, "usedBy");
  if (!sUsedBy.IsEmpty())
  {
    auto asset = ResolveAsset(sUsedBy);

    if (!asset.isValid())
    {
      ezStringBuilder sError;
      sError.SetFormat("No asset found for 'usedBy' filter '{}'. Pass a GUID or a path as returned by asset_find.", sUsedBy);
      out_result.SetError(sError);
      return;
    }

    ezSet<ezUuid> deps;
    CollectReferencedAssets(*asset, bTransitive, deps);

    if (bRestrictToSet)
    {
      // both reverse filters were given, so only assets satisfying both remain
      ezSet<ezUuid> intersection;
      for (const ezUuid& guid : deps)
      {
        if (allowedGuids.Contains(guid))
          intersection.Insert(guid);
      }
      allowedGuids.Swap(intersection);
    }
    else
    {
      bRestrictToSet = true;
      allowedGuids.Swap(deps);
    }
  }

  ezUInt32 uiTotalMatches = 0;

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.BeginArray("assets");

  {
    auto knownSubAssets = pCurator->GetKnownSubAssets();

    ezStringBuilder sIdentifier, sGuid, sTemp;

    for (auto it : *knownSubAssets)
    {
      const ezSubAsset& subAsset = it.Value();

      if (!subAsset.m_bMainAsset && !bIncludeSubAssets)
        continue;

      if (subAsset.m_pAssetInfo == nullptr)
        continue;

      if (bRestrictToSet && !allowedGuids.Contains(subAsset.m_Data.m_Guid))
        continue;

      subAsset.GetSubAssetIdentifier(sIdentifier);

      // ignore everything inside the asset cache, which is generated output rather than source assets
      if (sIdentifier.FindSubString("/AssetCache/") != nullptr)
        continue;

      if (!sPathFilter.IsEmpty() && !sIdentifier.StartsWith_NoCase(sPathFilter))
        continue;

      if (!sTypeFilter.IsEmpty())
      {
        sTemp.Set(";", subAsset.m_Data.m_sSubAssetsDocumentTypeName, ";");

        if (sTypeFilter.FindSubString_NoCase(sTemp) == nullptr)
          continue;
      }

      if (!sTagFilter.IsEmpty())
      {
        // an asset whose info failed to load carries no tags, so it cannot satisfy a tag filter
        const ezAssetDocumentInfo* pInfo = subAsset.m_pAssetInfo->m_Info.Borrow();

        if (pInfo == nullptr)
          continue;

        sTemp.Set(";", sTagFilter, ";");

        if (pInfo->GetAssetsDocumentTags().FindSubString_NoCase(sTemp) == nullptr)
          continue;
      }

      if (stateFilter != ezAssetInfo::TransformState::COUNT && subAsset.m_pAssetInfo->m_TransformState != stateFilter)
        continue;

      if (!searchFilter.IsEmpty())
      {
        // the same three-way match the asset browser does: path, then name, then GUID, so that a
        // pasted GUID and a partial name both find something
        if (!searchFilter.PassesFilters(sIdentifier) && !searchFilter.PassesFilters(subAsset.GetName()))
        {
          ezConversionUtils::ToString(subAsset.m_Data.m_Guid, sGuid);

          if (!searchFilter.PassesFilters(sGuid))
            continue;
        }
      }

      ++uiTotalMatches;

      if (uiTotalMatches > s_uiMaxAssetResults)
        continue;

      writer.BeginObject();
      WriteAssetIdentity(writer, subAsset);
      writer.AddVariableString("transformState", TransformStateToString(subAsset.m_pAssetInfo->m_TransformState));
      writer.EndObject();
    }
  }

  writer.EndArray();

  const ezUInt32 uiReturned = ezMath::Min(uiTotalMatches, s_uiMaxAssetResults);
  writer.AddVariableUInt32("totalMatches", uiTotalMatches);
  writer.AddVariableUInt32("returned", uiReturned);
  writer.AddVariableBool("truncated", uiTotalMatches > uiReturned);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("No project is open, so there is no asset database to query.");
    return;
  }

  const ezStringView sAsset = ezMcpJson::GetString(arguments, "asset");

  if (sAsset.IsEmpty())
  {
    out_result.SetError("The 'asset' argument is required. Pass a GUID or a path, as returned by asset_find.");
    return;
  }

  auto asset = ResolveAsset(sAsset);

  if (!asset.isValid())
  {
    ezStringBuilder sError;
    sError.SetFormat("No asset found for '{}'. Use asset_find to search for it.", sAsset);
    out_result.SetError(sError);
    return;
  }

  const bool bDependencies = ezMcpJson::GetBool(arguments, "dependencies", true);
  const ezAssetInfo* pAssetInfo = asset->m_pAssetInfo;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  WriteAssetIdentity(writer, *asset);

  writer.AddVariableString("absolutePath", pAssetInfo->m_Path.GetAbsolutePath());
  writer.AddVariableString("dataDirectory", pAssetInfo->m_Path.GetDataDir());
  writer.AddVariableString("transformState", TransformStateToString(pAssetInfo->m_TransformState));

  if (pAssetInfo->m_pDocumentTypeDescriptor != nullptr)
  {
    writer.AddVariableString("outputExtension", pAssetInfo->m_pDocumentTypeDescriptor->m_sResourceFileExtension);

    // the documentation for this asset's type, so an agent looking at one asset does not need a
    // separate asset_types call to find out what the type is for
    const ezString& sTypeName = pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;
    ezMcpTranslation::AddOptionalString(writer, "typeHelpUrl", ezMcpTranslation::GetHelpURL(sTypeName));
  }

  const ezAssetDocumentInfo* pInfo = pAssetInfo->m_Info.Borrow();

  if (pInfo != nullptr)
  {
    const ezString& sTags = pInfo->GetAssetsDocumentTags();
    if (!sTags.IsEmpty())
      writer.AddVariableString("tags", sTags);

    if (bDependencies)
    {
      WriteStringSet(writer, "transformDependencies", pInfo->m_TransformDependencies);
      WriteStringSet(writer, "thumbnailDependencies", pInfo->m_ThumbnailDependencies);
      WriteStringSet(writer, "packageDependencies", pInfo->m_PackageDependencies);
    }
  }

  // These say why an asset is in a broken state, so they are written regardless of the dependencies
  // flag - they are the answer the caller came for and are empty on a healthy asset anyway.
  WriteStringSet(writer, "missingTransformDependencies", pAssetInfo->m_MissingTransformDeps);
  WriteStringSet(writer, "missingThumbnailDependencies", pAssetInfo->m_MissingThumbnailDeps);
  WriteStringSet(writer, "missingPackageDependencies", pAssetInfo->m_MissingPackageDeps);
  WriteStringSet(writer, "circularDependencies", pAssetInfo->m_CircularDependencies);

  if (!pAssetInfo->m_SubAssets.IsEmpty())
  {
    writer.BeginArray("subAssets");
    for (const ezUuid& guid : pAssetInfo->m_SubAssets)
    {
      auto sub = pCurator->GetSubAsset(guid);

      writer.BeginObject();
      if (sub.isValid())
      {
        WriteAssetIdentity(writer, *sub);
      }
      else
      {
        // the sub-asset is listed but not in the table - report the guid rather than dropping it
        writer.AddVariableUuid("guid", guid);
      }
      writer.EndObject();
    }
    writer.EndArray();
  }

  // The transform log is what makes a failure diagnosable. It is only non-empty when something went
  // wrong, so healthy assets pay nothing for it.
  if (!pAssetInfo->m_LogEntries.IsEmpty())
  {
    writer.BeginArray("transformLog");
    for (const ezLogEntry& entry : pAssetInfo->m_LogEntries)
    {
      writer.BeginObject();
      writer.AddVariableString("severity", AssetLogSeverityToString(entry.m_Type));
      writer.AddVariableString("text", entry.m_sMsg);
      writer.EndObject();
    }
    writer.EndArray();
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteUses(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("No project is open, so there is no asset database to query.");
    return;
  }

  const ezStringView sAsset = ezMcpJson::GetString(arguments, "asset");

  if (sAsset.IsEmpty())
  {
    out_result.SetError("The 'asset' argument is required. Pass a GUID or a path, as returned by asset_find.");
    return;
  }

  auto asset = ResolveAsset(sAsset);

  if (!asset.isValid())
  {
    ezStringBuilder sError;
    sError.SetFormat("No asset found for '{}'. Use asset_find to search for it.", sAsset);
    out_result.SetError(sError);
    return;
  }

  const ezStringView sDirection = ezMcpJson::GetString(arguments, "direction", "both");
  const bool bTransitive = ezMcpJson::GetBool(arguments, "transitive", false);

  const bool bWantUses = sDirection.IsEqual_NoCase("uses") || sDirection.IsEqual_NoCase("both");
  const bool bWantUsedBy = sDirection.IsEqual_NoCase("usedBy") || sDirection.IsEqual_NoCase("both");

  if (!bWantUses && !bWantUsedBy)
  {
    ezStringBuilder sError;
    sError.SetFormat("'{}' is not a valid direction. Use 'uses', 'usedBy' or 'both'.", sDirection);
    out_result.SetError(sError);
    return;
  }

  const ezUuid assetGuid = asset->m_Data.m_Guid;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  WriteAssetIdentity(writer, *asset);
  writer.AddVariableBool("transitive", bTransitive);

  auto writeGuidArray = [&](ezStringView sName, const ezSet<ezUuid>& guids)
  {
    writer.BeginArray(sName);

    ezUInt32 uiWritten = 0;

    for (const ezUuid& guid : guids)
    {
      if (uiWritten >= s_uiMaxAssetResults)
        break;

      auto other = pCurator->GetSubAsset(guid);

      writer.BeginObject();
      if (other.isValid())
      {
        WriteAssetIdentity(writer, *other);
      }
      else
      {
        // a reference to something the curator does not know - report the guid rather than hiding it,
        // because a dangling reference is exactly what the caller is looking for
        writer.AddVariableUuid("guid", guid);
      }
      writer.EndObject();

      ++uiWritten;
    }

    writer.EndArray();

    ezStringBuilder sTotalName(sName, "Total");
    writer.AddVariableUInt32(sTotalName, guids.GetCount());

    if (guids.GetCount() > uiWritten)
    {
      ezStringBuilder sTruncatedName(sName, "Truncated");
      writer.AddVariableBool(sTruncatedName, true);
    }
  };

  if (bWantUses)
  {
    // who references this asset
    ezSet<ezUuid> uses;
    pCurator->FindAllUses(assetGuid, uses, bTransitive);
    uses.Remove(assetGuid);

    writeGuidArray("usedBy", uses);
  }

  if (bWantUsedBy)
  {
    // what this asset references
    ezSet<ezUuid> deps;
    CollectReferencedAssets(*asset, bTransitive, deps);

    writeGuidArray("uses", deps);
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteThumbnail(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("No project is open, so there is no asset database to query.");
    return;
  }

  const ezStringView sAsset = ezMcpJson::GetString(arguments, "asset");

  if (sAsset.IsEmpty())
  {
    out_result.SetError("The 'asset' argument is required. Pass a GUID or a path, as returned by asset_find.");
    return;
  }

  auto asset = ResolveAsset(sAsset);

  if (!asset.isValid())
  {
    ezStringBuilder sError;
    sError.SetFormat("No asset found for '{}'. Use asset_find to search for it.", sAsset);
    out_result.SetError(sError);
    return;
  }

  const ezAssetInfo* pAssetInfo = asset->m_pAssetInfo;
  const ezAssetDocumentTypeDescriptor* pTypeDesc = pAssetInfo->m_pDocumentTypeDescriptor;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  WriteAssetIdentity(writer, *asset);

  // Whether the type produces a thumbnail at all is a different answer from 'the file is missing',
  // and the caller has to be able to tell them apart before going looking for the file.
  const bool bMainAsset = asset->m_bMainAsset;
  bool bSupportsThumbnail = false;

  if (pTypeDesc != nullptr)
  {
    const ezBitflags<ezAssetDocumentFlags> flags = pTypeDesc->m_AssetDocumentFlags;

    bSupportsThumbnail = bMainAsset
                           ? flags.IsAnySet(ezAssetDocumentFlags::SupportsThumbnail | ezAssetDocumentFlags::AutoThumbnailOnTransform)
                           : flags.IsAnySet(ezAssetDocumentFlags::SubAssetsSupportThumbnail | ezAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform);
  }

  writer.AddVariableBool("supportsThumbnail", bSupportsThumbnail);

  ezAssetDocumentManager* pManager = pAssetInfo->m_pDocumentTypeDescriptor != nullptr
                                       ? static_cast<ezAssetDocumentManager*>(pAssetInfo->m_pDocumentTypeDescriptor->m_pManager)
                                       : nullptr;

  if (pManager == nullptr)
  {
    writer.AddVariableString("note", "The document manager for this asset type is unavailable, so no thumbnail path can be computed.");
    writer.EndObject();
    out_result.m_sText = writer.GetResult();
    return;
  }

  const ezString sThumbPath = pManager->GenerateResourceThumbnailPath(pAssetInfo->m_Path, bMainAsset ? ezStringView() : ezStringView(asset->m_Data.m_sName));

  // this is pure path arithmetic, so it returns a path whether or not anything was ever written there
  writer.AddVariableString("thumbnailPath", sThumbPath);
  writer.AddVariableBool("exists", ezOSFile::ExistsFile(sThumbPath));

  const bool bUpToDate = pManager->IsThumbnailUpToDate(pAssetInfo->m_Path, bMainAsset ? ezStringView() : ezStringView(asset->m_Data.m_sName),
    pAssetInfo->m_ThumbHash, pTypeDesc != nullptr && pTypeDesc->m_pDocumentType != nullptr ? pTypeDesc->m_pDocumentType->GetTypeVersion() : 0);

  writer.AddVariableBool("upToDate", bUpToDate);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteTransform(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("No project is open, so there is nothing to transform.");
    return;
  }

  const ezStringView sAsset = ezMcpJson::GetString(arguments, "asset");

  if (sAsset.IsEmpty())
  {
    out_result.SetError("The 'asset' argument is required. Pass a GUID or a path, as returned by asset_find.");
    return;
  }

  ezUuid assetGuid;
  ezStringBuilder sIdentifier;

  {
    auto asset = ResolveAsset(sAsset);

    if (!asset.isValid())
    {
      ezStringBuilder sError;
      sError.SetFormat("No asset found for '{}'. Use asset_find to search for it.", sAsset);
      out_result.SetError(sError);
      return;
    }

    assetGuid = asset->m_Data.m_Guid;
    asset->GetSubAssetIdentifier(sIdentifier);
  }
  // the curator lock is released here on purpose - TransformAsset takes it itself, and holding it
  // across a call that can run for minutes would block every other thread touching the curator

  // TriggeredManually is always set, because without it assets flagged OnlyTransformManually - scenes
  // among them - silently do nothing, which reads as an unexplained success to the caller.
  ezBitflags<ezTransformFlags> flags = ezTransformFlags::TriggeredManually;

  if (ezMcpJson::GetBool(arguments, "force", false))
    flags |= ezTransformFlags::ForceTransform;

  // This runs on the main thread and so does not go through the asset processor, meaning it produces
  // no progress event. Recording it here keeps asset_history a complete picture of what was
  // transformed rather than only what the background processor did.
  const ezTime startTime = ezTime::Now();
  const ezTransformStatus status = pCurator->TransformAsset(assetGuid, flags);
  RecordHistory(assetGuid, sIdentifier, status, ezTime::Now() - startTime, false);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableUuid("guid", assetGuid);
  writer.AddVariableString("path", sIdentifier);
  writer.AddVariableBool("succeeded", status.Succeeded());

  // NeedsImport is neither success nor failure - the asset has to be re-imported before it can be
  // transformed, which is a different action for the caller to take.
  switch (status.m_Result)
  {
    case ezTransformResult::Success:
      writer.AddVariableString("result", "Success");
      break;
    case ezTransformResult::Failure:
      writer.AddVariableString("result", "Failure");
      break;
    case ezTransformResult::NeedsImport:
      writer.AddVariableString("result", "NeedsImport");
      break;
  }

  if (!status.m_sMessage.IsEmpty())
    writer.AddVariableString("message", status.m_sMessage);

  // the state after the fact, which is what the caller would otherwise call asset_info for
  {
    auto asset = pCurator->GetSubAsset(assetGuid);

    if (asset.isValid() && asset->m_pAssetInfo != nullptr)
    {
      writer.AddVariableString("transformState", TransformStateToString(asset->m_pAssetInfo->m_TransformState));
    }
  }

  if (status.Failed())
    writer.AddVariableString("hint", "Call asset_info on this asset for the full transform log.");

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteProcessor(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetProcessor* pProcessor = ezAssetProcessor::GetSingleton();

  if (pProcessor == nullptr)
  {
    out_result.SetError("No project is open, so there is no asset processor.");
    return;
  }

  const ezStringView sAction = ezMcpJson::GetString(arguments, "action", "query");

  const bool bStart = sAction.IsEqual_NoCase("start");
  const bool bStop = sAction.IsEqual_NoCase("stop");
  const bool bForceStop = sAction.IsEqual_NoCase("forceStop");
  const bool bQuery = sAction.IsEqual_NoCase("query");

  if (!bStart && !bStop && !bForceStop && !bQuery)
  {
    ezStringBuilder sError;
    sError.SetFormat("'{}' is not a valid action. Use 'start', 'stop', 'forceStop' or 'query'.", sAction);
    out_result.SetError(sError);
    return;
  }

  if (bStart)
  {
    // The UI pairs the start with a file system check, so that assets changed while the processor was
    // off are noticed rather than sitting untouched (see ezQtCuratorControl::BackgroundProcessClicked).
    ezAssetCurator::GetSingleton()->CheckFileSystem();
    pProcessor->StartProcessor();
  }
  else if (bStop || bForceStop)
  {
    // Without the force flag the running processes are allowed to finish what they are on, which is
    // why 'stop' can leave the state at Stopping rather than Stopped.
    pProcessor->StopProcessor(bForceStop);
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("action", sAction);
  WriteProcessorStatus(writer, "processor");

  // The state is read back straight after the call and may still be in transition - StopProcessor
  // without force returns while processes are still finishing.
  if (bStop && pProcessor->GetProcessorState() == ezAssetProcessor::ProcessorState::Stopping)
  {
    writer.AddVariableString("note", "The processor is finishing the assets it had already started. Call again with 'forceStop' to kill those processes instead of waiting.");
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteHealth(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();

  if (pCurator == nullptr)
  {
    out_result.SetError("No project is open, so there is no asset database to query.");
    return;
  }

  ezUInt32 uiNumAssets = 0;
  ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> counts;

  pCurator->GetAssetTransformStats(uiNumAssets, counts);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableUInt32("totalAssets", uiNumAssets);

  // Only the non-zero states are written. On a healthy project this reduces the whole answer to the
  // asset count and one UpToDate entry, which is what makes this cheap enough to call routinely.
  writer.BeginObject("states");
  for (ezUInt32 i = 0; i < counts.GetCount() && i < ezAssetInfo::TransformState::COUNT; ++i)
  {
    if (counts[i] == 0)
      continue;

    writer.AddVariableUInt32(TransformStateToString(static_cast<ezAssetInfo::TransformState>(i)), counts[i]);
  }
  writer.EndObject();

  // The background processor. The state counts above are a snapshot that moves on its own while the
  // processor is running, so without this a caller cannot tell '485 assets are broken' from '485 have
  // not been got to yet' - which are very different answers.
  WriteProcessorStatus(writer, "processor");

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteListImporters(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sFile = ezMcpJson::GetString(arguments, "file");

  ezTempHybridArray<ezAssetDocumentGenerator*, 16> generators;
  ezAssetDocumentGenerator::CreateGenerators(generators);

  // The generators are freshly allocated and owned by this call.
  EZ_SCOPE_EXIT(ezAssetDocumentGenerator::DestroyGenerators(generators));

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.BeginArray("importers");

  ezUInt32 uiCount = 0;

  for (ezAssetDocumentGenerator* pGen : generators)
  {
    if (pGen == nullptr)
      continue;

    // SupportsFileType() takes either a path or a bare extension, so 'png' and 'C:/x/y.png' both work.
    if (!sFile.IsEmpty() && !pGen->SupportsFileType(sFile))
      continue;

    ezHybridArray<ezAssetDocumentGenerator::ImportMode, 4> modes;

    // An empty path asks the generator for its general purpose modes, which is what to report when no
    // particular file was named.
    pGen->GetImportModes(sFile, modes);

    if (modes.IsEmpty())
      continue;

    ++uiCount;

    writer.BeginObject();
    writer.AddVariableString("documentExtension", pGen->GetDocumentExtension());

    writer.BeginArray("modes");
    for (const ezAssetDocumentGenerator::ImportMode& mode : modes)
    {
      writer.BeginObject();
      writer.AddVariableString("name", mode.m_sName);

      // What the import dialog would preselect. An agent with no opinion should use the highest.
      switch (mode.m_Priority)
      {
        case ezAssetDocGeneratorPriority::HighPriority: writer.AddVariableString("priority", "high"); break;
        case ezAssetDocGeneratorPriority::DefaultPriority: writer.AddVariableString("priority", "default"); break;
        case ezAssetDocGeneratorPriority::LowPriority: writer.AddVariableString("priority", "low"); break;
        default: writer.AddVariableString("priority", "undecided"); break;
      }

      ezMcpTranslation::AddOptionalString(writer, "displayName", ezMcpTranslation::GetDisplayName(mode.m_sName));

      // What the mode actually does, where the name does not say it - which channel holds what in an
      // ORM texture, for instance. This is the part an agent cannot infer.
      ezMcpTranslation::AddOptionalString(writer, "description", ezMcpTranslation::GetTooltip(mode.m_sName));

      writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
  }

  writer.EndArray();
  writer.AddVariableUInt32("count", uiCount);

  // Without a file the list of every importable extension is the useful summary; with one it would
  // just repeat what the caller already knows.
  if (sFile.IsEmpty())
  {
    ezSet<ezString> extensions;
    ezAssetDocumentGenerator::GetSupportsFileTypes(extensions);

    writer.BeginArray("fileTypes");
    for (const ezString& sExt : extensions)
    {
      writer.WriteString(sExt);
    }
    writer.EndArray();
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpAssetTool::ExecuteImport(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sFile = ezMcpJson::GetString(arguments, "file");

  if (sFile.IsEmpty())
  {
    out_result.SetError("No 'file' argument given.");
    return;
  }

  ezStringBuilder sAbsFile = sFile;
  sAbsFile.MakeCleanPath();

  if (!ezPathUtils::IsAbsolutePath(sAbsFile))
  {
    ezStringBuilder sResolved = sAbsFile;

    if (ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sResolved, true))
    {
      sAbsFile = sResolved;
    }
    else
    {
      sResolved = sAbsFile;
      if (ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sResolved))
        sAbsFile = sResolved;
    }
  }

  // Still relative means no data directory claimed it. Checked before ExistsFile(), which asserts on a
  // relative path instead of returning false.
  if (!ezPathUtils::IsAbsolutePath(sAbsFile) || !ezOSFile::ExistsFile(sAbsFile))
  {
    ezStringBuilder s;
    s.SetFormat("There is no file at '{}'. Pass an absolute path, or one relative to the data directory parent such as "
                "'Testing Chambers/Textures/Wood.png'.",
      sAbsFile);
    out_result.SetError(s);
    return;
  }

  if (!ezToolsProject::IsProjectOpen())
  {
    out_result.SetError("No project is open, so nothing can be imported.");
    return;
  }

  // The generated document lands next to the source file, so a source outside the project would
  // create an asset outside it too, which the editor cannot then track.
  if (!ezToolsProject::GetSingleton()->IsDocumentInAllowedRoot(sAbsFile))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' is outside the open project. The imported asset is created next to its source file, so the source has to live "
                "in one of the project's data directories - 'project_info' lists them. Copy the file into the project first.",
      sAbsFile);
    out_result.SetError(s);
    return;
  }

  const ezStringView sRequestedMode = ezMcpJson::GetString(arguments, "mode");

  ezTempHybridArray<ezAssetDocumentGenerator*, 16> generators;
  ezAssetDocumentGenerator::CreateGenerators(generators);
  EZ_SCOPE_EXIT(ezAssetDocumentGenerator::DestroyGenerators(generators));

  ezAssetDocumentGenerator* pGenerator = nullptr;
  ezString sMode = sRequestedMode;

  if (sRequestedMode.IsEmpty())
  {
    // No mode named: pick the highest priority one any generator offers for this file, which is what
    // the import dialog preselects.
    ezAssetDocGeneratorPriority bestPriority = ezAssetDocGeneratorPriority::Undecided;

    for (ezAssetDocumentGenerator* pGen : generators)
    {
      if (pGen == nullptr || !pGen->SupportsFileType(sAbsFile))
        continue;

      ezHybridArray<ezAssetDocumentGenerator::ImportMode, 4> modes;
      pGen->GetImportModes(sAbsFile, modes);

      for (const ezAssetDocumentGenerator::ImportMode& mode : modes)
      {
        if (pGenerator == nullptr || mode.m_Priority > bestPriority)
        {
          pGenerator = pGen;
          bestPriority = mode.m_Priority;
          sMode = mode.m_sName;
        }
      }
    }

    if (pGenerator == nullptr)
    {
      ezStringBuilder s;
      s.SetFormat("No importer handles '{}'. 'asset_importers' lists the file types that can be imported.",
        ezPathUtils::GetFileExtension(sAbsFile));
      out_result.SetError(s);
      return;
    }
  }
  else
  {
    // A named mode has to belong to a generator that also accepts this file, otherwise Import() would
    // fail with a message about file types that does not mention the mode.
    for (ezAssetDocumentGenerator* pGen : generators)
    {
      if (pGen == nullptr || !pGen->SupportsFileType(sAbsFile))
        continue;

      ezHybridArray<ezAssetDocumentGenerator::ImportMode, 4> modes;
      pGen->GetImportModes(sAbsFile, modes);

      for (const ezAssetDocumentGenerator::ImportMode& mode : modes)
      {
        if (mode.m_sName == sRequestedMode)
        {
          pGenerator = pGen;
          break;
        }
      }

      if (pGenerator != nullptr)
        break;
    }

    if (pGenerator == nullptr)
    {
      ezStringBuilder s;
      s.SetFormat("'{}' is not an import mode available for '{}'. Call 'asset_importers' with that file to see which modes apply.",
        sRequestedMode, ezPathUtils::GetFileExtension(sAbsFile));
      out_result.SetError(s);
      return;
    }
  }

  // bOpenDocument false: opening is the caller's decision, and OpenDocumentQueued() would defer a
  // window creation past the end of this call where its failure could not be reported.
  ezAssetDocumentGenerator::ImportResult importResult = ezAssetDocumentGenerator::ImportResult::Imported;
  ezStringBuilder sTargetDoc;

  const ezStatus res = pGenerator->Import(sAbsFile, sMode, false, &importResult, &sTargetDoc);

  if (res.Failed())
  {
    ezStringBuilder s;
    s.SetFormat("Importing '{}' failed: {}", sAbsFile, res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("sourceFile", sAbsFile);
  writer.AddVariableString("mode", sMode);
  writer.AddVariableString("document", sTargetDoc);

  // The generators skip silently when the target exists, so without this the caller would read a
  // success and believe it had just imported something. Import() reports which it was.
  if (importResult == ezAssetDocumentGenerator::ImportResult::AlreadyExists)
  {
    writer.AddVariableBool("alreadyImported", true);
    writer.AddVariableString("note", "An asset for this source file already existed, so nothing was imported and the existing "
                                     "document is unchanged. Delete it first to re-import.");
  }
  else if (!ezOSFile::ExistsFile(sTargetDoc))
  {
    // Reported as created, but not where the target path says. A generator that produces several
    // documents can do this legitimately, so it is a note rather than a failure.
    writer.AddVariableBool("created", true);
    writer.AddVariableString("note", "The import succeeded but no document is at the expected path - the importer may have created "
                                     "several documents or named them differently. Use 'asset_find' to locate them.");
  }
  else
  {
    writer.AddVariableBool("created", true);
    writer.AddVariableString("note", "The document was created and saved but is not open. Use 'document_open' to adjust its "
                                     "properties, and 'asset_transform' to produce its runtime data.");
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
