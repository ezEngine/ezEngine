#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <QAbstractItemModel>

class EZ_EDITORFRAMEWORK_DLL ezAssetCuratorModel : public QAbstractItemModel
{
  Q_OBJECT

public:

  ezAssetCuratorModel(QObject* pParent);
  ~ezAssetCuratorModel();
  
  void resetModel();
  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }

public slots:

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
  void AssetCuratorEventHandler(const ezAssetCurator::Event& e);

  ezDeque<ezUuid> m_AssetsToDisplay;
};