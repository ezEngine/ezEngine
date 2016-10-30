#pragma once

#include <GuiFoundation/Basics.h>
#include <QSortFilterProxyModel>
#include <Foundation/Containers/Map.h>

class QWidget;

class EZ_GUIFOUNDATION_DLL ezQtTreeSearchFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  ezQtTreeSearchFilterModel(QWidget* parent);

  void SetFilterText(const QString& text);

protected:
  void RecomputeVisibleItems();
  bool UpdateVisibility(const QModelIndex& idx);
  virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

  QAbstractItemModel* m_pSourceModel;
  QString m_sFilterText;
  ezMap<QModelIndex, bool> m_Visible;
};


