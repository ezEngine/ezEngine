#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtNodeScene;
class ezQtNodeView;

class ezProceduralPlacementAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezProceduralPlacementAssetDocumentWindow(ezDocument* pDocument);
  ~ezProceduralPlacementAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "ProceduralPlacementAsset"; }
  virtual const char* GetWindowLayoutGroupName() const override { return "ProceduralPlacementAsset"; }

  private slots:


private:
  ezQtNodeScene* m_pScene;
  ezQtNodeView* m_pView;
};
