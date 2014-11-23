#pragma once

#include <EditorFramework/Plugin.h>
#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectTree.h>

class EZ_EDITORFRAMEWORK_DLL ezRawDocumentTreeModel : public QAbstractItemModel
{
public:
  ezRawDocumentTreeModel(const ezDocumentObjectTree* pTree);
  ~ezRawDocumentTreeModel();

  const ezDocumentObjectTree* GetDocumentTree() const { return m_pDocumentTree; }

  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &child) const override;

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
  virtual Qt::DropActions supportedDropActions() const override;

  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;
 
private:
  QModelIndex ComputeParent(const ezDocumentObjectBase* pObject) const;
  ezInt32 ComputeIndex(const ezDocumentObjectBase* pObject) const;
  QModelIndex ComputeModelIndex(const ezDocumentObjectBase* pObject) const;


  void TreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

  const ezDocumentObjectTree* m_pDocumentTree;

};

