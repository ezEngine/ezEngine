#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>

class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserFilter : public ezQtAssetFilter
{
  Q_OBJECT
public:
  explicit ezQtAssetBrowserFilter(QObject* pParent);

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() const { return m_bShowItemsInSubFolders; }

  void SetSortByRecentUse(bool bSort);
  bool GetSortByRecentUse() const { return m_bSortByRecentUse; }

  void SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_sTextFilter; }

  void SetPathFilter(const char* szPath);
  const char* GetPathFilter() const { return m_sPathFilter; }

  void SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

signals:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void ShowSubFolderItemsChanged();
  void SortByRecentUseChanged();

public:
  virtual bool IsAssetFiltered(const ezAssetInfo* pInfo) const override;
  virtual bool Less(ezAssetInfo* pInfoA, ezAssetInfo* pInfoB) const override;

private:
  ezString m_sTextFilter, m_sTypeFilter, m_sPathFilter;
  bool m_bShowItemsInSubFolders;
  bool m_bSortByRecentUse;
};
