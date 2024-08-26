#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtCustomDataAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtCustomDataAssetDocumentWindow(ezDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "CustomDataAsset"; }
};
