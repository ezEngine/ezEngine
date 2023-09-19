#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtSubstancePackageAssetWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtSubstancePackageAssetWindow(ezDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "SubstancePackageAsset"; }
};
