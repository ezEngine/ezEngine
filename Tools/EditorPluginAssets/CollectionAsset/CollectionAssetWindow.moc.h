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

  virtual const char* GetGroupName() const { return "CollectionAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
};