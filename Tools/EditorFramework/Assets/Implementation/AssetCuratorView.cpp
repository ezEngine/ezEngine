#include <PCH.h>
#include <EditorFramework/Assets/AssetCuratorView.moc.h>

ezAssetCuratorView::ezAssetCuratorView(QWidget* parent) : QListView(parent)
{
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setViewMode(QListView::ViewMode::IconMode);
  setUniformItemSizes(true);
  setResizeMode(QListView::ResizeMode::Adjust);
}


