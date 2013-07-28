#pragma once

#include <QTableView>

class InputListModel : public QAbstractTableModel 
{
  Q_OBJECT
 
public:
  InputListModel(QObject* parent = 0);

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;

  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;


};

