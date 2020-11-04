#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class ezQtNodeScene;
class ezQtNodeView;

class ezQtAnimationControllerAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtAnimationControllerAssetScene(QObject* parent = nullptr);
  ~ezQtAnimationControllerAssetScene();
};
