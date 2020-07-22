﻿#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

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
