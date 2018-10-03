#pragma once

#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>

class ezSkeletonAssetDocument;

class ezQtJointAdapter : public ezQtNamedAdapter
{
  Q_OBJECT;
public:
  ezQtJointAdapter(const ezSkeletonAssetDocument* pDocument);
  ~ezQtJointAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int row, int column, int role) const override;

private:
  const ezSkeletonAssetDocument* m_pDocument;
};
