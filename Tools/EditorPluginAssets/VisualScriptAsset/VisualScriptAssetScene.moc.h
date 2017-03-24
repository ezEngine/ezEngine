#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class ezQtNodeView;

class ezQtVisualScriptAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetScene(QObject* parent = nullptr);
  ~ezQtVisualScriptAssetScene();

protected:
  virtual void ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin) override;
};
