#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtBlackboardTemplateAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtBlackboardTemplateAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtBlackboardTemplateAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "BlackboardTemplateAsset"; }
};
