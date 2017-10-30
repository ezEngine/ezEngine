#pragma once

#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>

class ezSceneDocument;

class ezQtScenegraphModel : public ezQtGameObjectModel
{
  Q_OBJECT

public:

  ezQtScenegraphModel(ezSceneDocument* pDocument);
  ~ezQtScenegraphModel();

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private:
  ezSceneDocument* m_pSceneDocument;
};
