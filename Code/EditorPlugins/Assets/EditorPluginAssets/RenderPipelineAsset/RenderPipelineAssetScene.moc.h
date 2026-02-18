#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

class ezQtVisualGraphScene;
class ezQtVisualGraphView;

/// Qt scene for render pipeline asset editing.
///
/// Manages the visual scene for editing render pipeline assets in the editor.
class ezQtRenderPipelineAssetScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtRenderPipelineAssetScene(QObject* pParent = nullptr);
  ~ezQtRenderPipelineAssetScene();
};
