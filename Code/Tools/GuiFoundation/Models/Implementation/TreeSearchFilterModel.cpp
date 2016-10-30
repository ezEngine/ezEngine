#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <QWidget>

ezQtTreeSearchFilterModel::ezQtTreeSearchFilterModel(QWidget* parent)
  : QSortFilterProxyModel(parent)
{
  m_bIncludeChildren = false;
}

void ezQtTreeSearchFilterModel::SetFilterText(const QString& text)
{
  // only clear the current visible state, if the new filter text got shorter
  // this way previous information stays valid and can be used to early out
  if (!text.contains(m_sFilterText, Qt::CaseInsensitive))
    m_Visible.Clear();

  m_sFilterText = text;

  if (!m_sFilterText.isEmpty())
  {
    RecomputeVisibleItems();
  }

  invalidateFilter();
}


void ezQtTreeSearchFilterModel::SetIncludeChildren(bool bInclude)
{
  m_bIncludeChildren = true;

  if (!m_sFilterText.isEmpty())
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

  QString name = m_pSourceModel->data(idx, Qt::DisplayRole).toString();

  bool bSubTreeAnyVisible = false;

  if (name.contains(m_sFilterText, Qt::CaseInsensitive))
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

bool ezQtTreeSearchFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
  QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

  if (m_sFilterText.isEmpty())
    return true;

  auto itVis = m_Visible.Find(idx);

  return itVis.IsValid() && itVis.Value();
}

