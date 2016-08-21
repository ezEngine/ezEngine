#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezPropertyAnimAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezPropertyAnimAssetDocumentWindow(ezDocument* pDocument);
  ~ezPropertyAnimAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "PropertyAnimAsset"; }

};