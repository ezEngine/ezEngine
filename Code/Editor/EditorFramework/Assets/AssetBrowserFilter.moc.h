#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserFilter : public ezQtAssetFilter
{
  Q_OBJECT
public:
  explicit ezQtAssetBrowserFilter(QObject* pParent);

  /// Resets all filters to their default state.
  void Reset();
  void UpdateImportExtensions(const ezSet<ezString>& extensions);

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() const { return m_bShowItemsInSubFolders; }

  void SetShowFiles(bool bShow);
  bool GetShowFiles() const { return m_bShowFiles; }

  void SetShowNonImportableFiles(bool bShow);
  bool GetShowNonImportableFiles() const { return m_bShowNonImportableFiles; }

  void SetShowItemsInHiddenFolders(bool bShow);
  bool GetShowItemsInHiddenFolders() const { return m_bShowItemsInHiddenFolders; }

  void SetSortByRecentUse(bool bSort);
  virtual bool GetSortByRecentUse() const override { return m_bSortByRecentUse; }

  void SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_SearchFilter.GetSearchText(); }

  void SetPathFilter(const char* szPath);
  ezStringView GetPathFilter() const;

  void SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

  void SetFileExtensionFilters(ezStringView sExtensions);

  void SetRequiredTag(ezStringView sRequiredTag);

  /// \brief If set, the given item will be visible no matter what until any other filter is changed.
  /// This is used to ensure that newly created assets are always visible, even if they are excluded from the current filter.
  void SetTemporaryPinnedItem(ezStringView sDataDirParentRelativePath);
  ezStringView GetTemporaryPinnedItem() const { return m_sTemporaryPinnedItem; }

Q_SIGNALS:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void SortByRecentUseChanged();

public:
  virtual bool IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const override;

private:
  ezString m_sTypeFilter;
  ezString m_sRequiredTag = "*"; // show all is the default for the asset browser
  ezString m_sPathFilter;
  ezString m_sTemporaryPinnedItem;
  ezSearchPatternFilter m_SearchFilter;
  bool m_bShowItemsInSubFolders = true;
  bool m_bShowFiles = true;
  bool m_bShowNonImportableFiles = true;
  bool m_bShowItemsInHiddenFolders = false;
  bool m_bSortByRecentUse = false;
  mutable ezStringBuilder m_sTemp; // stored here to reduce unnecessary allocations

  // Cache for uses search
  bool m_bUsesSearchActive = false;
  bool m_bTransitive = false;
  ezSet<ezUuid> m_Uses;
  ezSet<ezString> m_ImportExtensions;
  ezSet<ezString> m_FileExtensions;
};
