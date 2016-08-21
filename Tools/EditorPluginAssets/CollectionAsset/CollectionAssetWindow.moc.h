#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezCollectionAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezCollectionAssetDocumentWindow(ezDocument* pDocument);
  ~ezCollectionAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "CollectionAsset"; }

};