#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetChecker.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

namespace
{
  struct AssetToCheck
  {
    ezUuid m_Guid;
    ezString m_sAbsPath;
    ezString m_sRelPath;
    ezString m_sDocTypeName;
  };
} // namespace

void ezAssetChecker::Run(const ezAssetCheckOptions& options, ezAssetCheckSummary& out_summary)
{
  ezSearchPatternFilter nameFilter;
  nameFilter.SetSearchText(options.m_sNameFilter);

  // Cache per-document-type whether any selected rule applies, so we don't re-check all rules for every asset.
  ezMap<ezString, bool> ruleAppliesToDocType;

  // 1. Snapshot the matching assets under the curator lock, then release it before opening documents.
  ezDynamicArray<AssetToCheck> assets;
  {
    auto pKnownAssets = ezAssetCurator::GetSingleton()->GetKnownAssets();

    for (auto it = pKnownAssets->GetIterator(); it.IsValid(); ++it)
    {
      const ezAssetInfo* pInfo = it.Value();
      if (pInfo == nullptr || pInfo->m_pDocumentTypeDescriptor == nullptr)
        continue;

      const ezString& sDocTypeName = pInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;

      if (!options.m_sDocumentTypeName.IsEmpty() && options.m_sDocumentTypeName != sDocTypeName)
        continue;

      const ezStringView sRelPath = pInfo->m_Path.GetDataDirParentRelativePath();

      if (!nameFilter.IsEmpty() && !nameFilter.PassesFilters(sRelPath))
        continue;

      // Skip assets that no selected rule would look at anyway.
      bool bAnyRuleApplies = false;
      if (auto itCached = ruleAppliesToDocType.Find(sDocTypeName); itCached.IsValid())
      {
        bAnyRuleApplies = itCached.Value();
      }
      else
      {
        for (const ezAssetCheckRule* pRule : options.m_Rules)
        {
          if (pRule->AppliesToDocumentType(sDocTypeName))
          {
            bAnyRuleApplies = true;
            break;
          }
        }

        ruleAppliesToDocType[sDocTypeName] = bAnyRuleApplies;
      }

      if (!bAnyRuleApplies)
        continue;

      AssetToCheck& a = assets.ExpandAndGetRef();
      a.m_Guid = it.Key();
      a.m_sAbsPath = pInfo->m_Path.GetAbsolutePath();
      a.m_sRelPath = sRelPath;
      a.m_sDocTypeName = sDocTypeName;
    }
  }

  if (assets.IsEmpty())
    return;

  ezProgressRange range("Checking Assets", assets.GetCount(), true);

  for (const AssetToCheck& asset : assets)
  {
    if (range.WasCanceled())
    {
      out_summary.m_bCanceled = true;
      break;
    }

    ezStringBuilder sFileName = asset.m_sRelPath;
    range.BeginNextStep(sFileName.GetFileNameAndExtension());

    ++out_summary.m_uiAssetsChecked;

    // 3. Open the document (reuse an already open one).
    bool bWasOpen = false;
    ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(asset.m_Guid);
    if (pDoc != nullptr)
    {
      bWasOpen = true;
    }
    else
    {
      pDoc = ezQtEditorApp::GetSingleton()->OpenDocument(asset.m_sAbsPath, ezDocumentFlags::None);
    }

    if (pDoc == nullptr)
    {
      ezAssetCheckResult& result = out_summary.m_Results.ExpandAndGetRef();
      result.m_AssetGuid = asset.m_Guid;
      result.m_sAssetPath = asset.m_sRelPath;
      result.m_sAbsAssetPath = asset.m_sAbsPath;
      ezAssetCheckNote& note = result.m_Notes.ExpandAndGetRef();
      note.m_Severity = ezAssetCheckSeverity::Error;
      note.m_sMessage = "Could not open asset document.";
      ++out_summary.m_uiErrors;
      continue;
    }

    const bool bHadUnsavedChanges = pDoc->IsModified();

    ezDynamicArray<ezAssetCheckNote> notes;
    ezUInt32 uiTotalFixes = 0;

    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();

    // 5. Run each rule that applies to this document type.
    for (ezAssetCheckRule* pRule : options.m_Rules)
    {
      if (!pRule->AppliesToDocumentType(asset.m_sDocTypeName))
        continue;

      ezAssetCheckContext ctx;
      ctx.m_pDocument = pDoc;
      ctx.m_pNotes = &notes;
      ctx.m_uiFixCount = 0;
      ctx.m_bAutoFix = options.m_bAutoFix && pRule->CanFix();

      if (ctx.m_bAutoFix)
      {
        ezStringBuilder sTransaction;
        sTransaction.SetFormat("Asset Check: {}", pRule->GetDisplayName());
        pAcc->StartTransaction(sTransaction);

        pRule->CheckDocument(ctx);

        if (ctx.m_uiFixCount > 0)
          pAcc->FinishTransaction();
        else
          pAcc->CancelTransaction();
      }
      else
      {
        pRule->CheckDocument(ctx);
      }

      uiTotalFixes += ctx.m_uiFixCount;
    }

    // 6. Save if the document was modified by a fix.
    bool bSaved = false;
    if (uiTotalFixes > 0)
    {
      if (bHadUnsavedChanges)
      {
        ezAssetCheckNote& note = notes.ExpandAndGetRef();
        note.m_Severity = ezAssetCheckSeverity::Warning;
        note.m_sMessage = "Document had unsaved changes; fixes were applied but the document was not saved automatically.";
      }
      else
      {
        const ezStatus res = pDoc->SaveDocument(true);
        if (res.Failed())
        {
          ezStringBuilder sMsg;
          sMsg.SetFormat("Failed to save document: {}", res.GetMessageString());
          ezAssetCheckNote& note = notes.ExpandAndGetRef();
          note.m_Severity = ezAssetCheckSeverity::Error;
          note.m_sMessage = sMsg;
        }
        else
        {
          bSaved = true;
          ++out_summary.m_uiSaved;
        }
      }
    }

    // 7. Close the document if we opened it and no window is using it.
    if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
    {
      pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }

    // 8. Accumulate counts and record the result if there is anything to report.
    if (!notes.IsEmpty())
    {
      ezAssetCheckResult& result = out_summary.m_Results.ExpandAndGetRef();
      result.m_AssetGuid = asset.m_Guid;
      result.m_sAssetPath = asset.m_sRelPath;
      result.m_sAbsAssetPath = asset.m_sAbsPath;
      result.m_Notes = notes;
      result.m_bSaved = bSaved;

      for (const ezAssetCheckNote& note : notes)
      {
        if (note.m_Severity == ezAssetCheckSeverity::Error)
          ++out_summary.m_uiErrors;
        else
          ++out_summary.m_uiWarnings;

        if (note.m_bFixed)
          ++out_summary.m_uiFixed;
      }
    }
  }
}
