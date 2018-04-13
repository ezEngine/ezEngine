#pragma once

#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezSkeletonAssetDocument;

class ezQtSkeletonModel : public QAbstractItemModel
{
public:
  ezQtSkeletonModel(QWidget* parent, const ezSkeletonAssetDocument* pDocument);
  ~ezQtSkeletonModel();

  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &child) const override;

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

protected:
  const ezSkeletonAssetDocument* m_pDocument = nullptr;
  const ezDocumentObjectManager* m_pManager = nullptr;
};

