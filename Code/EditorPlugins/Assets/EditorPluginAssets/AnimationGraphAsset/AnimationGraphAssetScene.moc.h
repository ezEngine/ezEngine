#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class ezQtVisualGraphScene;
class ezQtVisualGraphView;

class ezQtAnimationGraphAssetScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtAnimationGraphAssetScene(QObject* pParent = nullptr);
  ~ezQtAnimationGraphAssetScene();
};
