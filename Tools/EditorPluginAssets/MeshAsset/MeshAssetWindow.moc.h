#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezMeshAssetDocumentWindow : public ezDocumentWindow
{
  Q_OBJECT

public:
  ezMeshAssetDocumentWindow(ezDocumentBase* pDocument);
  ~ezMeshAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "MeshAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);


};