#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class ezQtNodeScene;
class ezQtNodeView;

class ezQtRenderPipelineAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtRenderPipelineAssetScene(QObject* parent = nullptr);
  ~ezQtRenderPipelineAssetScene();

};
