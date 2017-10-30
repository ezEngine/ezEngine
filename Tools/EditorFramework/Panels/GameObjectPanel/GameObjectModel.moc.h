#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <EditorFramework/Document/GameObjectDocument.h>

class ezSceneDocument;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:

  ezQtGameObjectModel(ezGameObjectDocument* pDocument, const char* szRootProperty = "Children");
  ~ezQtGameObjectModel();

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

protected:
  virtual void TreeEventHandler(const ezDocumentObjectStructureEvent& e) override;

private:
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void GameObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezGameObjectMetaData>::EventData& e);

  ezGameObjectDocument* m_pGameObjectDocument;
};
