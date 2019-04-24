#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezProceduralPlacementAssetDocument;

class ezQtNodeScene;
class ezQtNodeView;

class ezProceduralPlacementAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezProceduralPlacementAssetDocumentWindow(ezProceduralPlacementAssetDocument* pDocument);
  ~ezProceduralPlacementAssetDocumentWindow();

  ezProceduralPlacementAssetDocument* GetProceduralPlacementDocument();

  virtual const char* GetWindowLayoutGroupName() const override { return "ProceduralPlacementAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezQtNodeScene* m_pScene;
  ezQtNodeView* m_pView;
};
