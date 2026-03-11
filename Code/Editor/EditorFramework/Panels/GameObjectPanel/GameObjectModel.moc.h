#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class ezSceneDocument;


/// \brief Custom delegate for game objects, used in ezQtGameObjectWidget.
///
/// Renders additional icons to display stats.
class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectDelegate : public ezQtItemDelegate
{
  Q_OBJECT
public:
  ezQtGameObjectDelegate(QObject* pParent, ezGameObjectDocument* pDocument);
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect GetHiddenIconRect(const QStyleOptionViewItem& opt);
  static QRect GetActiveParentIconRect(const QStyleOptionViewItem& opt);

  ezGameObjectDocument* m_pDocument = nullptr;
};

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectAdapter : public ezQtNameableAdapter
{
  Q_OBJECT;

public:
  enum UserRoles
  {
    HiddenRole = Qt::UserRole + 0,
    ActiveParentRole = Qt::UserRole + 1,
  };

  ezQtGameObjectAdapter(ezDocumentObjectManager* pObjectManager, ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>* pObjectMetaData = nullptr, ezObjectMetaData<ezUuid, ezGameObjectMetaData>* pGameObjectMetaData = nullptr);
  ~ezQtGameObjectAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool setData(const ezDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

public:
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void GameObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezGameObjectMetaData>::EventData& e);

protected:
  ezDocumentObjectManager* m_pObjectManager = nullptr;
  ezGameObjectDocument* m_pGameObjectDocument = nullptr;
  ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>* m_pObjectMetaData = nullptr;
  ezObjectMetaData<ezUuid, ezGameObjectMetaData>* m_pGameObjectMetaData = nullptr;
  ezEventSubscriptionID m_GameObjectMetaDataSubscription;
  ezEventSubscriptionID m_DocumentObjectMetaDataSubscription;
};

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:
  ezQtGameObjectModel(const ezDocumentObjectManager* pObjectManager, const ezUuid& root = ezUuid());
  ~ezQtGameObjectModel();
};
