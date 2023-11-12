#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class ezQtNodeScene;
class ezQtNodeView;

class ezQtAnimationGraphAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtAnimationGraphAssetScene(QObject* pParent = nullptr);
  ~ezQtAnimationGraphAssetScene();
};
