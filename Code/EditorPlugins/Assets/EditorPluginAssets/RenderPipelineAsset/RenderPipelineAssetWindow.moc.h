#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class ezQtVisualGraphScene;
class ezQtVisualGraphView;

class ezQtRenderPipelineAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtRenderPipelineAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtRenderPipelineAssetDocumentWindow();

private Q_SLOTS:

private:
  // Both are owned by Qt: the scene through its parent, the view through the panel it is set on.
  ezQtVisualGraphScene* m_pScene = nullptr;
  ezQtVisualGraphView* m_pView = nullptr;
};
