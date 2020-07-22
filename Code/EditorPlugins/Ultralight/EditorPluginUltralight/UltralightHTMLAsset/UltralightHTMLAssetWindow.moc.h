#pragma once

#include <EditorPluginUltralight/UltralightHTMLAsset/UltralightHTMLAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezUltralightHTMLAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezUltralightHTMLAssetDocumentWindow(ezDocument* pDocument);
  ~ezUltralightHTMLAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "UltralightHTMLAsset"; }
  virtual const char* GetWindowLayoutGroupName() const override { return "UltralightHTMLAsset"; }

private slots:


private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezUltralightHTMLAssetDocument* m_pAssetDoc;
  QLabel* m_pLabelInfo;
};
