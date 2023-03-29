#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>

class ezScene2Document;
struct ezScene2LayerEvent;
struct ezDocumentEvent;

/// \brief Custom adapter for layers, used in ezQtLayerPanel.
class EZ_EDITORPLUGINSCENE_DLL ezQtLayerAdapter : public ezQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  ezQtLayerAdapter(ezScene2Document* pDocument);
  ~ezQtLayerAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool setData(const ezDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

  enum UserRoles
  {
    LayerGuid = Qt::UserRole + 0,
  };

private:
  void LayerEventHandler(const ezScene2LayerEvent& e);
  void DocumentEventHander(const ezDocumentEvent& e);

private:
  ezScene2Document* m_pSceneDocument;
  ezEvent<const ezScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  ezEvent<const ezDocumentEvent&>::Unsubscriber m_DocumentEventUnsubscriber;
  ezUuid m_CurrentActiveLayer;
};

/// \brief Custom delegate for layers, used in ezQtLayerPanel.
/// Provides buttons to toggle the layer visible / loaded states.
/// Relies on ezQtLayerAdapter to trigger updates and provide the LayerGuid.
class ezQtLayerDelegate : public ezQtItemDelegate
{
  Q_OBJECT
public:
  ezQtLayerDelegate(QObject* pParent, ezScene2Document* pDocument);

  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect GetVisibleIconRect(const QStyleOptionViewItem& opt);
  static QRect GetLoadedIconRect(const QStyleOptionViewItem& opt);

  bool m_bPressed = false;
  ezScene2Document* m_pDocument = nullptr;
};

/// \brief Custom model for layers, used in ezQtLayerPanel.
class ezQtLayerModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:
  ezQtLayerModel(ezScene2Document* pDocument);
  ~ezQtLayerModel() = default;

private:
  ezScene2Document* m_pDocument = nullptr;
};
