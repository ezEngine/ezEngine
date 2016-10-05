#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtCollectionAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtCollectionAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtCollectionAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "CollectionAsset"; }

};