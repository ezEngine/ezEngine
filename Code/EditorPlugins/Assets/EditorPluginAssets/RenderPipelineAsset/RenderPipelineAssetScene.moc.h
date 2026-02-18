#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class ezQtVisualGraphScene;
class ezQtVisualGraphView;

class ezQtRenderPipelineAssetScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtRenderPipelineAssetScene(QObject* pParent = nullptr);
  ~ezQtRenderPipelineAssetScene();
};
