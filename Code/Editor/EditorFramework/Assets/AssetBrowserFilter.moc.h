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

  void SetShowPluginDataDirs(bool bShow);
  bool GetShowPluginDataDirs() const { return m_bShowPluginDataDirs; }

  /// The names of the top level folders of those data directories that are provided by the active plugin bundles.
  ///
  /// These are the first path segment of a ezDataDirPath::GetDataDirParentRelativePath(), i.e. the folder name of the
  /// data directory root, not a full path. While GetShowPluginDataDirs() is disabled, everything under them is hidden.
  void SetPluginDataDirNames(const ezSet<ezString>& names);
  const ezSet<ezString>& GetPluginDataDirNames() const { return m_PluginDataDirNames; }

  /// Whether the given path lies in a data directory that comes from an active plugin bundle.
  bool IsInPluginDataDir(ezStringView sDataDirParentRelativePath) const;

  void SetSortByRecentUse(bool bSort);
  virtual bool GetSortByRecentUse() const override { return m_bSortByRecentUse; }

  void SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_SearchFilter.GetSearchText(); }

  void SetPathFilter(const char* szPath);
  ezStringView GetPathFilter() const;

  void SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

  /// The type filter string that stands for 'no type restriction at all'.
  ///
  /// Needed to tell an actually restricting type filter apart from one that just lists every known type,
  /// which is what the 'all assets' entry of the type combo box sets.
  void SetAllTypesFilter(ezStringView sTypes);

  /// Whether any filter is active that makes the browser show only a subset of what is in the current folder.
  bool IsFilterActive() const;

  void SetFileExtensionFilters(ezStringView sExtensions);

  void SetRequiredTag(ezStringView sRequiredTag);

  /// If set, the given item will be visible no matter what until any other filter is changed.
  /// This is used to ensure that newly created assets are always visible, even if they are excluded from the current filter.
  void SetTemporaryPinnedItem(ezStringView sDataDirParentRelativePath);
  ezStringView GetTemporaryPinnedItem() const { return m_sTemporaryPinnedItem; }

Q_SIGNALS:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void SortByRecentUseChanged();

  /// Emitted when the set of data directories that are excluded changed.
  ///
  /// Unlike FilterChanged(), which fires for every filter of any kind, this only fires when which folders exist at
  /// all changed, so listeners that rebuild folder structures don't do so on every keystroke.
  void PluginDataDirsChanged();

public:
  virtual ezAssetFilterResult IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const override;

private:
  /// Whether the given path is hidden because of the folder it lies in, or - for a folder - because of itself.
  ///
  /// Folders starting with a dot and '*_data' folders are treated as hidden. bIsFolder has to be set for a path that
  /// names a folder, otherwise a folder called '*_data' is not recognized, because only its content would be checked.
  bool IsInHiddenFolder(ezStringView sDataDirParentRelativePath, bool bIsFolder) const;

  ezString m_sTypeFilter;
  ezString m_sAllTypesFilter;
  ezString m_sRequiredTag = "*"; // show all is the default for the asset browser
  ezString m_sPathFilter;
  ezString m_sTemporaryPinnedItem;
  ezSearchPatternFilter m_SearchFilter;
  bool m_bShowItemsInSubFolders = true;
  bool m_bShowFiles = true;
  bool m_bShowNonImportableFiles = true;
  bool m_bShowItemsInHiddenFolders = false;
  bool m_bShowPluginDataDirs = false;
  bool m_bSortByRecentUse = false;
  mutable ezStringBuilder m_sTemp; // stored here to reduce unnecessary allocations

  // Cache for uses search
  bool m_bUsesSearchActive = false;
  bool m_bTransitive = false;
  ezSet<ezUuid> m_Uses;

  ezSet<ezString> m_PluginDataDirNames;
  ezSet<ezString> m_ImportExtensions;
  ezSet<ezString> m_FileExtensions;
};
