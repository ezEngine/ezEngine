#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class ezSceneDocument;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectAdapter : public ezQtNameableAdapter
{
  Q_OBJECT;

public:
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
