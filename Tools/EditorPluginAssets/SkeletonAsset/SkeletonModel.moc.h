#pragma once

#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>

class ezSkeletonAssetDocument;

class ezQtBoneAdapter : public ezQtNamedAdapter
{
  Q_OBJECT;
public:
  ezQtBoneAdapter(const ezSkeletonAssetDocument* pDocument);
  ~ezQtBoneAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int column, int role) const override;

private:
  const ezSkeletonAssetDocument* m_pDocument;
};
