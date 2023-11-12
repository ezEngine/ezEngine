#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <QWidget>

ezQtTreeSearchFilterModel::ezQtTreeSearchFilterModel(QWidget* pParent)
  : QSortFilterProxyModel(pParent)
{
  m_bIncludeChildren = false;
}

void ezQtTreeSearchFilterModel::SetFilterText(const QString& sText)
{
  // only clear the current visible state, if the new filter text got shorter
  // this way previous information stays valid and can be used to early out
  if (!sText.contains(m_Filter.GetSearchText().GetData(), Qt::CaseInsensitive) || m_Filter.ContainsExclusions())
    m_Visible.Clear();

  m_Filter.SetSearchText(sText.toUtf8().data());

  if (!m_Filter.IsEmpty())
  {
    RecomputeVisibleItems();
  }

  invalidateFilter();
}


void ezQtTreeSearchFilterModel::SetIncludeChildren(bool bInclude)
{
  m_bIncludeChildren = true;

  if (!m_Filter.IsEmpty())
  {
    RecomputeVisibleItems();
    invalidateFilter();
  }
}

void ezQtTreeSearchFilterModel::RecomputeVisibleItems()
{
  m_pSourceModel = sourceModel();

  const int numRows = m_pSourceModel->rowCount();

  for (int r = 0; r < numRows; ++r)
  {
    QModelIndex idx = m_pSourceModel->index(r, 0);

    UpdateVisibility(idx, false);
  }
}

bool ezQtTreeSearchFilterModel::UpdateVisibility(const QModelIndex& idx, bool bParentIsVisible)
{
  bool bExisted = false;
  auto itVis = m_Visible.FindOrAdd(idx, &bExisted);

  // early out, if we already know that this object is invisible (from previous searches)
  if (bExisted && !itVis.Value())
    return false;

  QString displayName = m_pSourceModel->data(idx, Qt::DisplayRole).toString();
  QVariant internalNameVar = m_pSourceModel->data(idx, Qt::UserRole + 1);

  bool bSubTreeAnyVisible = false;

  if (m_Filter.PassesFilters(displayName.toUtf8().data()) ||
      (internalNameVar.canConvert<QString>() && m_Filter.PassesFilters(internalNameVar.toString().toUtf8().data())))
  {
    bParentIsVisible = true;
    bSubTreeAnyVisible = true;
  }

  const int numRows = m_pSourceModel->rowCount(idx);

  for (int r = 0; r < numRows; ++r)
  {
    QModelIndex idxChild = m_pSourceModel->index(r, 0, idx);

    if (UpdateVisibility(idxChild, bParentIsVisible))
      bSubTreeAnyVisible = true;
  }

  if (m_bIncludeChildren && bParentIsVisible)
  {
    itVis.Value() = true;
  }
  else
  {
    itVis.Value() = bSubTreeAnyVisible;
  }

  return bSubTreeAnyVisible;
}

bool ezQtTreeSearchFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
  QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

  if (m_Filter.IsEmpty())
    return true;

  auto itVis = m_Visible.Find(idx);

  return itVis.IsValid() && itVis.Value();
}
