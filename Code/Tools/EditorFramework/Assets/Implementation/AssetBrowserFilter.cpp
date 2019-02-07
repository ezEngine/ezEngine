#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>


ezQtAssetBrowserFilter::ezQtAssetBrowserFilter(QObject* pParent)
    : ezQtAssetFilter(pParent)
{
}


void ezQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetShowItemsInHiddenFolders(false);
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

  Q_EMIT FilterChanged();
}

void ezQtAssetBrowserFilter::SetShowItemsInHiddenFolders(bool bShow)
{
  if (m_bShowItemsInHiddenFolders == bShow)
    return;

  m_bShowItemsInHiddenFolders = bShow;

  Q_EMIT FilterChanged();
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

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT TextFilterChanged();
}

void ezQtAssetBrowserFilter::SetPathFilter(const char* szPath)
{
  ezStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();

  if (m_sPathFilter == sCleanText)
    return;

  m_sPathFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT PathFilterChanged();
}

void ezQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  Q_EMIT FilterChanged();
  Q_EMIT TypeFilterChanged();
}

bool ezQtAssetBrowserFilter::IsAssetFiltered(const ezSubAsset* pInfo) const
{
  if (!m_sPathFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (!pInfo->m_pAssetInfo->m_sDataDirRelativePath.StartsWith_NoCase(m_sPathFilter))
      return true;

    if (!m_bShowItemsInSubFolders)
    {
      // do we find another path separator after the prefix path?
      // if so, there is a sub-folder, and thus we ignore it
      if (ezStringUtils::FindSubString(pInfo->m_pAssetInfo->m_sDataDirRelativePath.GetData() + m_sPathFilter.GetElementCount() + 1, "/") !=
          nullptr)
        return true;
    }
  }

  if (!m_bShowItemsInHiddenFolders)
  {
    if (ezStringUtils::FindSubString_NoCase(pInfo->m_pAssetInfo->m_sDataDirRelativePath.GetData() + m_sPathFilter.GetElementCount() + 1,
                                            "_data/") != nullptr)
      return true;
  }

  if (!m_sTextFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (pInfo->m_pAssetInfo->m_sDataDirRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
    {
      if (pInfo->GetName().FindSubString_NoCase(m_sTextFilter) == nullptr)
      {
        ezConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
        if (m_sTemp.FindSubString_NoCase(m_sTextFilter) == nullptr)
          return true;

        // we could actually (partially) match the GUID
      }
    }
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    m_sTemp.Set(";", pInfo->m_Data.m_sAssetTypeName, ";");

    if (!m_sTypeFilter.FindSubString(m_sTemp))
      return true;
  }
  return false;
}

bool ezQtAssetBrowserFilter::Less(const ezSubAsset* pInfoA, const ezSubAsset* pInfoB) const
{
  if (m_bSortByRecentUse && pInfoA->m_LastAccess.GetSeconds() != pInfoB->m_LastAccess.GetSeconds())
  {
    return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
  }

  ezStringView sSortA = pInfoA->GetName();
  ezStringView sSortB = pInfoB->GetName();

  ezInt32 iValue = ezStringUtils::Compare_NoCase(sSortA.GetData(), sSortB.GetData(), sSortA.GetData() + sSortA.GetElementCount(),
                                                 sSortB.GetData() + sSortB.GetElementCount());
  if (iValue == 0)
  {
    return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
  }
  return iValue < 0;
}
