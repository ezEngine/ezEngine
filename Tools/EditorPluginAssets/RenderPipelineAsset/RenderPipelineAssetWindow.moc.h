#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtRenderPipelineAssetScene;
class ezQtNodeView;

class ezQtRenderPipelineAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtRenderPipelineAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtRenderPipelineAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "RenderPipelineAsset"; }

private slots:
  
private:
  ezQtRenderPipelineAssetScene* m_pScene;
  ezQtNodeView* m_pView;
};