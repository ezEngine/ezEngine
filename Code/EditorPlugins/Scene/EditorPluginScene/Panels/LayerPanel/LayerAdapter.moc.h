#pragma once
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>

class ezScene2Document;
struct ezScene2LayerEvent;
struct ezDocumentEvent;

class ezQtLayerAdapter : public ezQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  ezQtLayerAdapter(ezScene2Document* pDocument);
  ~ezQtLayerAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int row, int column, int role) const override;
  virtual bool setData(const ezDocumentObject* pObject, int row, int column, const QVariant& value, int role) const override;

private:
  void LayerEventHandler(const ezScene2LayerEvent& e);
  void DocumentEventHander(const ezDocumentEvent& e);

private:
  ezScene2Document* m_pSceneDocument;
  ezEvent<const ezScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  ezEvent<const ezDocumentEvent&>::Unsubscriber m_DocumentEventUnsubscriber;
};

class ezQtLayerModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:
  ezQtLayerModel(ezScene2Document* pDocument);
  ~ezQtLayerModel() = default;

  virtual bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

private:
  ezScene2Document* m_pDocument = nullptr;
};
