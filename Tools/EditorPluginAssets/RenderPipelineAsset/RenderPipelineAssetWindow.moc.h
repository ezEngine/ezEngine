#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtNodeScene;
class ezQtNodeView;

class ezRenderPipelineAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezRenderPipelineAssetDocumentWindow(ezDocument* pDocument);
  ~ezRenderPipelineAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "RenderPipelineAsset"; }

private slots:
  
private:
  ezQtNodeScene* m_pScene;
  ezQtNodeView* m_pView;
};