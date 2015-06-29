#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <QAbstractItemModel>

class EZ_EDITORFRAMEWORK_DLL ezAssetBrowserModel : public QAbstractItemModel
{
  Q_OBJECT

public:

  ezAssetBrowserModel(QObject* pParent);
  ~ezAssetBrowserModel();
  
  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() { return m_bShowItemsInSubFolders; }

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

private slots:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  bool m_bIconMode;
  bool m_bShowItemsInSubFolders;
  void AssetCuratorEventHandler(const ezAssetCurator::Event& e);

  struct AssetEntry
  {
    ezString m_sSortingKey;
    ezUuid m_Guid;

    inline bool operator<(const AssetEntry& rhs) const
    {
      return m_sSortingKey < rhs.m_sSortingKey;
    }
  };

  ezString m_sTextFilter, m_sTypeFilter, m_sPathFilter;
  ezDeque<AssetEntry> m_AssetsToDisplay;
};