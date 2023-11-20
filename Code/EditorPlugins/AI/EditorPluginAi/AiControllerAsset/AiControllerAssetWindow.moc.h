#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class ezQtVisualGraphScene;
class ezQtVisualGraphView;

class ezQtAiControllerAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtAiControllerAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtAiControllerAssetDocumentWindow();

private:
  ezQtVisualGraphScene* m_pScene = nullptr;
  ezQtVisualGraphView* m_pView = nullptr;
};
