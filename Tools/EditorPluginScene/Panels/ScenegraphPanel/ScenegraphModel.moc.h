#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class ezSceneDocument;

class ezQtScenegraphModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:

  ezQtScenegraphModel(ezSceneDocument* pDocument);
  ~ezQtScenegraphModel();

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


private:
  void ObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e);

  ezSceneDocument* m_pSceneDocument;
};