#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

#include <QSortFilterProxyModel>

class QWidget;

class EZ_GUIFOUNDATION_DLL ezQtTreeSearchFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  /// Return true to indicate that the model index shall be visible.
  using CustomFilterFunc = ezDelegate<bool(QModelIndex, const ezSearchPatternFilter&)>;

  ezQtTreeSearchFilterModel(QWidget* pParent);

  void SetFilterText(const QString& sText);

  /// \brief By default only nodes (and their parents) are shown that fit the search criterion.
  /// If this is enabled, all child nodes of nodes that fit the criterion are included as well.
  void SetIncludeChildren(bool bInclude);

  /// The custom function is executed when when visibility is updated. Return false to hide an element.
  void SetCustomFilterFunc(CustomFilterFunc func);

protected:
  void RecomputeVisibleItems();
  bool UpdateVisibility(const QModelIndex& idx, bool bParentIsVisible);
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

  bool m_bIncludeChildren;
  QAbstractItemModel* m_pSourceModel;
  ezSearchPatternFilter m_Filter;
  ezMap<QModelIndex, bool> m_Visible;
  CustomFilterFunc m_CustomFilterFunc;
};
