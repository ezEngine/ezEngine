#pragma once

#include <EditorFramework/Assets/AssetCheckRule.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Deque.h>

/// Input for ezAssetChecker::Run.
struct ezAssetCheckOptions
{
  ezDynamicArray<ezAssetCheckRule*> m_Rules; ///< Rules to run. Owned by the caller (the dialog).
  ezString m_sDocumentTypeName;              ///< If non-empty, only assets of this document type are checked.
  ezString m_sNameFilter;                    ///< ezSearchPatternFilter, matched against the data-dir-relative asset path.
  bool m_bAutoFix = false;                   ///< If true, rules that CanFix() apply fixes and modified documents are saved.
};

/// Result for a single asset that produced at least one note.
struct ezAssetCheckResult
{
  ezUuid m_AssetGuid;
  ezString m_sAssetPath;    ///< Data-dir-relative path, for display.
  ezString m_sAbsAssetPath; ///< Absolute path, for opening on double-click.
  ezDynamicArray<ezAssetCheckNote> m_Notes;
  bool m_bSaved = false;
};

/// Aggregated outcome of a check run.
struct ezAssetCheckSummary
{
  ezDeque<ezAssetCheckResult> m_Results; ///< Only assets that produced notes.
  ezUInt32 m_uiAssetsChecked = 0;
  ezUInt32 m_uiWarnings = 0;
  ezUInt32 m_uiErrors = 0;
  ezUInt32 m_uiFixed = 0;
  ezUInt32 m_uiSaved = 0;
  bool m_bCanceled = false;
};

/// Runs a set of ezAssetCheckRules over the assets selected by ezAssetCheckOptions.
///
/// Run is UI-free (it only drives the global ezProgress, which surfaces a modal progress dialog in
/// the editor) so that a headless tool could call it as well. It must run on the main thread
/// because it opens, modifies and saves documents.
class EZ_EDITORFRAMEWORK_DLL ezAssetChecker
{
public:
  static void Run(const ezAssetCheckOptions& options, ezAssetCheckSummary& out_summary);
};
