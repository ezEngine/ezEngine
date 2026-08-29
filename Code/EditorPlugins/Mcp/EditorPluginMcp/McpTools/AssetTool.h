#pragma once

#include <Mcp/McpTool.h>

#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>

struct ezSubAsset;
struct ezAssetProcessorProgressEvent;
class ezMcpJsonWriter;

/// Queries the asset database: which asset types exist, which assets exist, and how they relate.
///
/// The tools split along the cost of answering. 'asset_types' and 'asset_health' are type-level and
/// aggregate views that stay cheap on any project. 'asset_find' is the listing half - it narrows
/// thousands of assets down to a handful of GUIDs - and 'asset_info', 'asset_uses' and
/// 'asset_thumbnail' are the detail halves that an agent calls on those few.
///
/// 'asset_transform' is the only one here that changes anything, and the only one slow enough that a
/// caller has to think about the timeout (see Status.md, 'Tool calls block the editor').
class ezMcpAssetTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpAssetTool, ezMcpToolProvider);

public:
  virtual void OnActivate() override;
  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  /// One finished asset transform, as recorded by asset_history.
  struct HistoryEntry
  {
    ezUInt64 m_uiId = 0;
    ezUuid m_Guid;
    ezString m_sPath;
    ezString m_sMessage;
    ezTime m_Duration;
    ezTime m_EndTime;
    ezEnum<ezTransformResult> m_Result;
    bool m_bBackground = true; ///< false for a transform this plugin triggered through asset_transform
  };

  void ProcessorProgressEventHandler(const ezAssetProcessorProgressEvent& e);
  void RecordHistory(const ezUuid& guid, ezStringView sPath, const ezTransformStatus& status, ezTime duration, bool bBackground);

  void ExecuteListTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteFind(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteUses(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteThumbnail(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteTransform(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteHealth(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteProcessor(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteHistory(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteListImporters(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteImport(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// Writes the identifying fields of one asset: guid, path, type and name. This is what a listing
  /// returns per entry and what every detail tool repeats at the top of its result, so an agent can
  /// tell which asset it is looking at without correlating against an earlier call.
  static void WriteAssetIdentity(ezMcpJsonWriter& ref_writer, const ezSubAsset& subAsset);

  /// How many finished transforms are kept. A full transform of a large project runs into the
  /// thousands, so this covers recent activity rather than the whole run.
  static constexpr ezUInt32 s_uiMaxHistoryEntries = 500;

  /// Progress events arrive from the processor's worker threads, everything else runs on the main
  /// thread. Guards m_History and m_uiNextHistoryId.
  mutable ezMutex m_HistoryMutex;
  ezDeque<HistoryEntry> m_History;
  ezEventSubscriptionID m_ProgressSubscription = 0;

  /// Assigned in order and never reused, so a 'sinceId' from an earlier call stays meaningful even
  /// once that entry has fallen out of the buffer. Same contract as log_read.
  ezUInt64 m_uiNextHistoryId = 1;
};
