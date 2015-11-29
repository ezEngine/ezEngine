#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezSurfaceAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezSurfaceAssetDocumentWindow(ezDocument* pDocument);
  ~ezSurfaceAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "SurfaceAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
};