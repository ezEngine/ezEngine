#include <PCH.h>
#include <EditorFramework/Assets/AssetCuratorView.moc.h>

ezAssetCuratorView::ezAssetCuratorView(QWidget* parent) : QListView(parent)
{
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setViewMode(QListView::ViewMode::ListMode);
  setUniformItemSizes(true);
}


