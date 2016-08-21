#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezSurfaceAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezSurfaceAssetDocumentWindow(ezDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "SurfaceAsset"; }

};