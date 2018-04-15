#pragma once

#include <EditorFramework/Plugin.h>
#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

/// \brief Adapter that defines data for specific type in the ezQtDocumentTreeModel.
///
/// Adapters are defined for a given type and define the property for child elements (needs to be array or set).
/// Furthermore they implement various model functions that will be redirected to it by the model for
/// objects of the given type.
class EZ_EDITORFRAMEWORK_DLL ezQtDocumentTreeModelAdapter : public QObject
{
  Q_OBJECT;
public:
  /// \brief Constructor. If m_sChildProperty is empty, this type does not have children.
  ezQtDocumentTreeModelAdapter(const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty);
  virtual const ezRTTI* GetType() const;
  virtual const ezString& GetChildProperty() const;

  virtual QVariant data(const ezDocumentObject* pObject, int column, int role = Qt::DisplayRole) const = 0;
  virtual bool setData(const ezDocumentObject* pObject, int column, const QVariant& value, int role) const;
  virtual Qt::ItemFlags flags(const ezDocumentObject* pObject, int column) const;

signals:
  void dataChanged(const ezDocumentObject* pObject, QVector<int> roles);

protected:
  const ezDocumentObjectManager* m_pTree = nullptr;
  const ezRTTI* m_pType = nullptr;
  ezString m_sChildProperty;
};

/// \brief Convenience class that returns the typename as Qt::DisplayRole.
/// Use this for testing or for the document root that can't be seen and is just for defining the hierarchy.
///
/// Example:
/// ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children");
class EZ_EDITORFRAMEWORK_DLL ezQtDummyAdapter : public ezQtDocumentTreeModelAdapter
{
  Q_OBJECT;
public:
  ezQtDummyAdapter(const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty);

  virtual QVariant data(const ezDocumentObject* pObject, int column, int role) const override;
};

/// \brief Convenience class that implements getting the name via a property on the object.
class EZ_EDITORFRAMEWORK_DLL ezQtNamedAdapter : public ezQtDocumentTreeModelAdapter
{
  Q_OBJECT;
public:
  ezQtNamedAdapter(const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty, const char* szNameProperty);
  ~ezQtNamedAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int column, int role) const override;

protected:
  virtual void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

protected:
  ezString m_sNameProperty;
};

/// \brief Convenience class that implements setting the name via a property on the object.
class EZ_EDITORFRAMEWORK_DLL ezQtNameableAdapter : public ezQtNamedAdapter
{
  Q_OBJECT;
public:
  ezQtNameableAdapter(const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty, const char* szNameProperty);
  ~ezQtNameableAdapter();
  virtual bool setData(const ezDocumentObject* pObject, int column, const QVariant& value, int role) const override;
  virtual Qt::ItemFlags flags(const ezDocumentObject* pObject, int column) const;
};

/// \brief Model that maps a document to a qt tree model.
///
/// Hierarchy is defined by ezQtDocumentTreeModelAdapter that have to be added via AddAdapter.
class EZ_EDITORFRAMEWORK_DLL ezQtDocumentTreeModel : public QAbstractItemModel
{
public:
  ezQtDocumentTreeModel(const ezDocumentObjectManager* pTree);
  ~ezQtDocumentTreeModel();

  const ezDocumentObjectManager* GetDocumentTree() const { return m_pDocumentTree; }
  /// \brief Adds an adapter. There can only be one adapter for any object type.
  /// Added adapters are taken ownership of by the model.
  void AddAdapter(ezQtDocumentTreeModelAdapter* adapter);
  /// \brief Returns the QModelIndex for the given object.
  /// Returned value is invalid if object is not mapped in model.
  QModelIndex ComputeModelIndex(const ezDocumentObject* pObject) const;

  /// \brief Enable drag&drop support, disabled by default.
  void SetAllowDragDrop(bool bAllow);

public: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &child) const override;

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  virtual Qt::DropActions supportedDropActions() const override;

  virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

protected:
  virtual void TreeEventHandler(const ezDocumentObjectStructureEvent& e);

private:
  QModelIndex ComputeParent(const ezDocumentObject* pObject) const;
  ezInt32 ComputeIndex(const ezDocumentObject* pObject) const;

  const ezQtDocumentTreeModelAdapter* GetAdapter(const ezRTTI* pType) const;

protected:
  const ezDocumentObjectManager* m_pDocumentTree = nullptr;
  ezHashTable<const ezRTTI*, ezQtDocumentTreeModelAdapter*> m_Adapters;
  bool m_bAllowDragDrop = false;
};

