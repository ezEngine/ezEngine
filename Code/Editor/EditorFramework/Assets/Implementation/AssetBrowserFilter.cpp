#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>


ezQtAssetBrowserFilter::ezQtAssetBrowserFilter(QObject* pParent)
  : ezQtAssetFilter(pParent)
{
  Reset();
}


void ezQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetShowFiles(true);
  SetShowNonImportableFiles(true);
  SetShowItemsInHiddenFolders(false);
  SetShowPluginDataDirs(false);
  SetSortByRecentUse(false);
  SetTextFilter("");
  SetTypeFilter("");
  SetPathFilter("");
}

void ezQtAssetBrowserFilter::UpdateImportExtensions(const ezSet<ezString>& extensions)
{
  m_ImportExtensions = extensions;
  if (!m_bShowNonImportableFiles)
    Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowItemsInSubFolders = bShow;

  Q_EMIT FilterChanged();
}


void ezQtAssetBrowserFilter::SetShowFiles(bool bShow)
{
  if (m_bShowFiles == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowFiles = bShow;

  Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetShowNonImportableFiles(bool bShow)
{
  if (m_bShowNonImportableFiles == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowNonImportableFiles = bShow;

  Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetShowItemsInHiddenFolders(bool bShow)
{
  if (m_bShowItemsInHiddenFolders == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowItemsInHiddenFolders = bShow;

  Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetShowPluginDataDirs(bool bShow)
{
  if (m_bShowPluginDataDirs == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowPluginDataDirs = bShow;

  Q_EMIT FilterChanged();
  Q_EMIT PluginDataDirsChanged();
}

void ezQtAssetBrowserFilter::SetPluginDataDirNames(const ezSet<ezString>& names)
{
  if (m_PluginDataDirNames == names)
    return;

  m_PluginDataDirNames = names;

  // while plugin data directories are shown, their names have no influence on what is visible
  if (!m_bShowPluginDataDirs)
  {
    m_sTemporaryPinnedItem.Clear();
    Q_EMIT FilterChanged();
    Q_EMIT PluginDataDirsChanged();
  }
}

void ezQtAssetBrowserFilter::SetSortByRecentUse(bool bSort)
{
  if (m_bSortByRecentUse == bSort)
    return;

  m_bSortByRecentUse = bSort;

  Q_EMIT FilterChanged();
  Q_EMIT SortByRecentUseChanged();
}


void ezQtAssetBrowserFilter::SetTextFilter(const char* szText)
{
  ezStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();
  sCleanText.ReplaceAll("*", "");

  if (m_SearchFilter.GetSearchText() == sCleanText)
    return;

  m_SearchFilter.SetSearchText(sCleanText);
  // Clear uses search cache
  m_bUsesSearchActive = false;
  m_bTransitive = false;
  m_Uses.Clear();

  const char* szRefGuid = ezStringUtils::FindSubString_NoCase(szText, "ref:");
  const char* szRefAllGuid = ezStringUtils::FindSubString_NoCase(szText, "ref-all:");
  if (szRefGuid || szRefAllGuid)
  {
    bool bTransitive = szRefAllGuid != nullptr;
    const char* szGuid = szRefAllGuid ? szRefAllGuid + strlen("ref-all:") : szRefGuid + strlen("ref:");
    if (ezConversionUtils::IsStringUuid(szGuid))
    {
      m_bUsesSearchActive = true;
      m_bTransitive = bTransitive;
      ezUuid guid = ezConversionUtils::ConvertStringToUuid(szGuid);
      ezAssetCurator::GetSingleton()->FindAllUses(guid, m_Uses, m_bTransitive);
    }
  }

  Q_EMIT FilterChanged();
  Q_EMIT TextFilterChanged();
}

void ezQtAssetBrowserFilter::SetPathFilter(const char* szPath)
{
  ezStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();
  // The assumption is that only full directory names are set as path filters. Thus, we can ensure they end with a / to make it easier to filter items inside the path.
  if (!sCleanText.IsEmpty() && !sCleanText.EndsWith_NoCase("/"))
  {
    sCleanText.Append("/");
  }

  if (m_sPathFilter == sCleanText)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sPathFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT PathFilterChanged();
}


ezStringView ezQtAssetBrowserFilter::GetPathFilter() const
{
  if (m_sPathFilter.EndsWith_NoCase("/"))
  {
    return m_sPathFilter.GetSubString(0, m_sPathFilter.GetCharacterCount() - 1);
  }
  return m_sPathFilter;
}

void ezQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sTypeFilter = szTypes;

  Q_EMIT FilterChanged();
  Q_EMIT TypeFilterChanged();
}

void ezQtAssetBrowserFilter::SetAllTypesFilter(ezStringView sTypes)
{
  m_sAllTypesFilter = sTypes;
}

bool ezQtAssetBrowserFilter::IsFilterActive() const
{
  if (!m_SearchFilter.IsEmpty())
    return true;

  if (!m_FileExtensions.IsEmpty())
    return true;

  // a type filter that lists every known type doesn't restrict anything
  if (!m_sTypeFilter.IsEmpty() && m_sTypeFilter != m_sAllTypesFilter)
    return true;

  return false;
}

void ezQtAssetBrowserFilter::SetFileExtensionFilters(ezStringView sExtensions)
{
  m_FileExtensions.Clear();

  ezTempHybridArray<ezStringView, 8> filters;
  sExtensions.Split(false, filters, ";", "*", ".");

  ezStringBuilder tmp;
  for (ezStringView filter : filters)
  {
    tmp = filter;
    tmp.ToLower();
    m_FileExtensions.Insert(tmp);
  }

  Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetRequiredTag(ezStringView sRequiredTag)
{
  ezStringBuilder tag;

  if (sRequiredTag == "*")
  {
    tag = "*";
  }
  else if (!sRequiredTag.IsEmpty())
  {
    tag.Set(";", sRequiredTag, ";");
  }
  // else: tag stays empty

  if (m_sRequiredTag == tag)
    return;

  m_sRequiredTag = tag;

  Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetTemporaryPinnedItem(ezStringView sDataDirParentRelativePath)
{
  if (m_sTemporaryPinnedItem == sDataDirParentRelativePath)
    return;

  m_sTemporaryPinnedItem = sDataDirParentRelativePath;
  Q_EMIT FilterChanged();
}

bool ezQtAssetBrowserFilter::IsInHiddenFolder(ezStringView sDataDirParentRelativePath, bool bIsFolder) const
{
  // treat folders starting with a dot as hidden folders
  if (sDataDirParentRelativePath.FindSubString("/."))
    return true;

  // '_data' folders hold the generated files of an asset and are hidden as well.
  // The 'uses' search is an exception: it looks for references to a specific asset, and those often live in '_data' folders.
  if (!(m_bUsesSearchActive && !m_SearchFilter.IsEmpty()))
  {
    // skip the path filter prefix, it is the folder the user explicitly navigated to and may itself be a '_data' folder
    const ezUInt32 uiSkip = ezMath::Min<ezUInt32>(m_sPathFilter.GetElementCount() + 1, sDataDirParentRelativePath.GetElementCount());
    const char* szSearchStart = sDataDirParentRelativePath.GetStartPointer() + uiSkip;

    // A folder path has no trailing separator, so searching for '_data/' only ever finds a parent folder. The folder
    // itself has to be checked separately, otherwise '_data' folders would still be listed as folders.
    if (bIsFolder && ezStringUtils::EndsWith_NoCase(szSearchStart, "_data", sDataDirParentRelativePath.GetEndPointer()))
      return true;

    if (ezStringUtils::FindSubString_NoCase(szSearchStart, "_data/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
      return true;
  }

  return false;
}

bool ezQtAssetBrowserFilter::IsInPluginDataDir(ezStringView sDataDirParentRelativePath) const
{
  if (m_PluginDataDirNames.IsEmpty())
    return false;

  // the first path segment is the folder name of the data directory root
  const char* szSep = sDataDirParentRelativePath.FindSubString("/");
  const ezStringView sDataDirName = szSep != nullptr ? ezStringView(sDataDirParentRelativePath.GetStartPointer(), szSep) : sDataDirParentRelativePath;

  return m_PluginDataDirNames.Contains(sDataDirName);
}

ezAssetFilterResult ezQtAssetBrowserFilter::IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const
{
  // ignore all paths leading into the AssetCache
  if (sDataDirParentRelativePath.FindSubString("/AssetCache/"))
    return ezAssetFilterResult::Filtered;

  // also ignore the AssetCache folder directly
  if (bIsFolder && sDataDirParentRelativePath.GetFileNameAndExtension() == "AssetCache")
    return ezAssetFilterResult::Filtered;

  if (sDataDirParentRelativePath == m_sTemporaryPinnedItem)
    return ezAssetFilterResult::Visible;

  // Data directories provided by plugins are hidden entirely, including their own root folder. They are not reported
  // as a separate exclusion reason, because the folder tree hides them as well, so there is no folder to browse into
  // whose item count could be shown.
  if (!m_bShowPluginDataDirs && IsInPluginDataDir(sDataDirParentRelativePath))
    return ezAssetFilterResult::Filtered;

  ezStringBuilder sExt;

  // Whether this is a plain file, i.e. not an asset and not a folder. Those are hidden by the type combo box unless
  // it is set to show files, so the check is deferred to the end, where it can be reported as its own exclusion reason.
  const bool bIsPlainFile = !pInfo && !bIsFolder;

  // if the string is not found in the path, ignore this asset
  if (!sDataDirParentRelativePath.StartsWith(m_sPathFilter))
    return ezAssetFilterResult::Filtered;

  // when any filter is active, we search through sub-folders, otherwise only the current folder is listed
  const bool bRecurseIntoSubFolders = IsFilterActive() || m_bShowItemsInSubFolders;

  if (bIsFolder)
  {
    // do we find another path separator after the prefix path?
    // if so, there is a sub-folder, and thus we ignore it
    if (ezStringUtils::FindSubString(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount(), "/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
    {
      return ezAssetFilterResult::Filtered;
    }
  }
  else if (!bRecurseIntoSubFolders)
  {
    // do we find another path separator after the prefix path?
    // if so, there is a sub-folder, and thus we ignore it
    if (ezStringUtils::FindSubString(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount(), "/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
    {
      return ezAssetFilterResult::Filtered;
    }
  }

  if (!m_SearchFilter.IsEmpty())
  {
    if (m_bUsesSearchActive)
    {
      if (pInfo == nullptr)
        return ezAssetFilterResult::Filtered;

      if (!m_Uses.Contains(pInfo->m_Data.m_Guid))
        return ezAssetFilterResult::Filtered;
    }
    else
    {
      // if the string is not found in the path, ignore this asset
      if (m_SearchFilter.PassesFilters(sDataDirParentRelativePath) == false)
      {
        if (pInfo == nullptr)
          return ezAssetFilterResult::Filtered;

        if (m_SearchFilter.PassesFilters(pInfo->GetName()) == false)
        {
          ezConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
          if (m_SearchFilter.PassesFilters(m_sTemp) == false)
            return ezAssetFilterResult::Filtered;

          // we could actually (partially) match the GUID
        }
      }
    }
  }

  // Always show folders on the right
  if (bIsFolder)
  {
    // unless we have a type filter active
    if (!m_sTypeFilter.IsEmpty())
      return ezAssetFilterResult::Filtered;

    // folders are never counted as hidden-folder matches, only the items inside them are
    return !m_bShowItemsInHiddenFolders && IsInHiddenFolder(sDataDirParentRelativePath, true) ? ezAssetFilterResult::Filtered : ezAssetFilterResult::Visible;
  }

  if (!m_FileExtensions.IsEmpty())
  {
    sExt = sDataDirParentRelativePath.GetFileExtension();
    sExt.ToLower();

    if (!m_FileExtensions.Contains(sExt))
      return ezAssetFilterResult::Filtered;
  }

  if (!m_sTypeFilter.IsEmpty() && pInfo != nullptr)
  {
    m_sTemp.Set(";", pInfo->m_Data.m_sSubAssetsDocumentTypeName, ";");

    if (!m_sTypeFilter.FindSubString(m_sTemp))
      return ezAssetFilterResult::Filtered;
  }

  // A plain file is no asset, so an active asset type filter can never match it. Selecting an asset type in the combo
  // box also turns m_bShowFiles off, so this is handled below as its own exclusion reason rather than here.
  if (!m_sTypeFilter.IsEmpty() && bIsPlainFile && m_bShowFiles)
    return ezAssetFilterResult::Filtered;

  if (pInfo && m_sRequiredTag != "*") // '*' means everything is allowed
  {
    const auto& tags = pInfo->m_pAssetInfo->m_Info->GetAssetsDocumentTags();

    if (m_sRequiredTag.IsEmpty())
    {
      // if the required tag is empty, we only display assets without any tags
      // so the "default tag" (nothing at all) is already a tag for not-tagged items
      // if you really want to see all assets, use * as the required tag
      if (!tags.IsEmpty())
        return ezAssetFilterResult::Filtered;
    }
    else
    {
      // otherwise search for ";required;" in the tags string (note the semicolons at the start and end as delimiters
      if (tags.FindSubString_NoCase(m_sRequiredTag) == nullptr)
        return ezAssetFilterResult::Filtered;
    }
  }

  // At this point the item passes every filter that has no switch of its own. What is left are the exclusions that the
  // user can toggle, each of which is reported separately so that the browser can say how many items it is hiding.

  if (bIsPlainFile && !m_bShowFiles)
    return ezAssetFilterResult::NonAssetFile;

  if (bIsPlainFile && !m_bShowNonImportableFiles)
  {
    sExt = sDataDirParentRelativePath.GetFileExtension();
    sExt.ToLower();

    if (!m_ImportExtensions.Contains(sExt))
      return ezAssetFilterResult::NonImportableFile;
  }

  if (!m_bShowItemsInHiddenFolders && IsInHiddenFolder(sDataDirParentRelativePath, false))
    return ezAssetFilterResult::HiddenFolder;

  return ezAssetFilterResult::Visible;
}
