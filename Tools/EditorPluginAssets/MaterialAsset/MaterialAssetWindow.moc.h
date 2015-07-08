#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezMaterialAssetDocumentWindow : public ezDocumentWindow
{
  Q_OBJECT

public:
  ezMaterialAssetDocumentWindow(ezDocumentBase* pDocument);
  ~ezMaterialAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "MaterialAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  QtImageWidget* m_pImageWidget;
};