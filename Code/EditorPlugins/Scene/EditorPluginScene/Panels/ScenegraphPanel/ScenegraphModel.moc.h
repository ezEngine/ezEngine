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

private:
  ezSceneDocument* m_pSceneDocument;
};
