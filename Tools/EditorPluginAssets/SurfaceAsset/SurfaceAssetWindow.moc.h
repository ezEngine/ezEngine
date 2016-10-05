#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtSurfaceAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtSurfaceAssetDocumentWindow(ezDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "SurfaceAsset"; }

};