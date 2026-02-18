#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

class ezQtVisualGraphScene;
class ezQtVisualGraphView;

/// Qt scene for animation graph asset editing.
///
/// Manages the visual scene for editing animation graph assets in the editor.
class ezQtAnimationGraphAssetScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtAnimationGraphAssetScene(QObject* pParent = nullptr);
  ~ezQtAnimationGraphAssetScene();
};
