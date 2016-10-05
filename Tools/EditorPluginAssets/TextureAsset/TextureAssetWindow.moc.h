#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class ezQtImageWidget;

class ezQtTextureAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtTextureAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtTextureAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "TextureAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezQtImageWidget* m_pImageWidget;
};