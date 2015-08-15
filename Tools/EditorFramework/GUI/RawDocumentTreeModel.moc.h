#pragma once

#include <EditorFramework/Plugin.h>
#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class EZ_EDITORFRAMEWORK_DLL ezRawDocumentTreeModel : public QAbstractItemModel
{
public:
  ezRawDocumentTreeModel(const ezDocumentObjectManager* pTree, const ezRTTI* pBaseClass, const char* szChildProperty);
  ~ezRawDocumentTreeModel();

  const ezDocumentObjectManager* GetDocumentTree() const { return m_pDocumentTree; }

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
 
  QModelIndex ComputeModelIndex(const ezDocumentObjectBase* pObject) const;
private:
  QModelIndex ComputeParent(const ezDocumentObjectBase* pObject) const;
  ezInt32 ComputeIndex(const ezDocumentObjectBase* pObject) const;
  
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

private:
  const ezDocumentObjectManager* m_pDocumentTree;
  const ezRTTI* m_pBaseClass;
  ezString m_sChildProperty;
  ezPropertyPath m_PropertyPath;
};

