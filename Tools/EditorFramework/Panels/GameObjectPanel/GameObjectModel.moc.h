#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <EditorFramework/Document/GameObjectDocument.h>

class ezSceneDocument;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectAdapter : public ezQtNameableAdapter
{
  Q_OBJECT;
public:
  ezQtGameObjectAdapter(ezGameObjectDocument* pDocument);
  ~ezQtGameObjectAdapter();
  virtual QVariant data(const ezDocumentObject* pObject, int row, int column, int role) const override;
  virtual bool setData(const ezDocumentObject* pObject, int row, int column, const QVariant& value, int role) const override;

private:
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void GameObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezGameObjectMetaData>::EventData& e);

private:
  ezGameObjectDocument* m_pGameObjectDocument;
};

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:

  ezQtGameObjectModel(ezGameObjectDocument* pDocument);
  ~ezQtGameObjectModel();
};
