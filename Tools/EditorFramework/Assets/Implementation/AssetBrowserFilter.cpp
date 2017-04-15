#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>


ezQtAssetBrowserFilter::ezQtAssetBrowserFilter(QObject* pParent)
  : ezQtAssetFilter(pParent)
{
}


void ezQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetSortByRecentUse(false);
  SetTextFilter("");
  SetTypeFilter("");
  SetPathFilter("");
}

void ezQtAssetBrowserFilter::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_bShowItemsInSubFolders = bShow;

  emit FilterChanged();
  emit ShowSubFolderItemsChanged();
}

void ezQtAssetBrowserFilter::SetSortByRecentUse(bool bSort)
{
  if (m_bSortByRecentUse == bSort)
    return;

  m_bSortByRecentUse = bSort;

  emit FilterChanged();
  emit SortByRecentUseChanged();
}


void ezQtAssetBrowserFilter::SetTextFilter(const char* szText)
{
  ezStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;

  emit FilterChanged();
  emit TextFilterChanged();
}

void ezQtAssetBrowserFilter::SetPathFilter(const char* szPath)
{
  ezStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();

  if (m_sPathFilter == sCleanText)
    return;

  m_sPathFilter = sCleanText;

  emit FilterChanged();
  emit PathFilterChanged();
}

void ezQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  emit FilterChanged();
  emit TypeFilterChanged();
}

bool ezQtAssetBrowserFilter::IsAssetFiltered(const ezAssetInfo* pInfo) const
{
  if (!m_sPathFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (!pInfo->m_sDataDirRelativePath.StartsWith_NoCase(m_sPathFilter))
      return true;

    if (!m_bShowItemsInSubFolders)
    {
      // do we find another path separator after the prefix path?
      // if so, there is a sub-folder, and thus we ignore it
      if (ezStringUtils::FindSubString(pInfo->m_sDataDirRelativePath.GetData() + m_sPathFilter.GetElementCount() + 1, "/") != nullptr)
        return true;
    }
  }

  if (!m_sTextFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (pInfo->m_sDataDirRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
    {
      ezConversionUtils::ToString(pInfo->m_Info.m_DocumentID, m_sTemp);

      if (m_sTemp.FindSubString(m_sTextFilter) == nullptr)
        return true;

      // we could actually (partially) match the GUID
    }
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    m_sTemp.Set(";", pInfo->m_Info.m_sAssetTypeName, ";");

    if (!m_sTypeFilter.FindSubString(m_sTemp))
      return true;
  }
  return false;
}

bool ezQtAssetBrowserFilter::Less(ezAssetInfo* pInfoA, ezAssetInfo* pInfoB) const
{
  if (m_bSortByRecentUse && pInfoA->m_LastAccess.GetSeconds() != pInfoB->m_LastAccess.GetSeconds())
  {
    return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
  }

  ezStringView sSortA = ezPathUtils::GetFileName(pInfoA->m_sDataDirRelativePath.GetData(), pInfoA->m_sDataDirRelativePath.GetData() + pInfoA->m_sDataDirRelativePath.GetElementCount());
  ezStringView sSortB = ezPathUtils::GetFileName(pInfoB->m_sDataDirRelativePath.GetData(), pInfoB->m_sDataDirRelativePath.GetData() + pInfoB->m_sDataDirRelativePath.GetElementCount());

  ezInt32 iValue = ezStringUtils::Compare_NoCase(sSortA.GetData(), sSortB.GetData(),
    sSortA.GetData() + sSortA.GetElementCount(), sSortB.GetData() + sSortB.GetElementCount());
  if (iValue == 0)
  {
    return pInfoA->m_Info.m_DocumentID < pInfoB->m_Info.m_DocumentID;
  }
  return iValue < 0;
}
