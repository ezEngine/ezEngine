#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Communication/Event.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class ezImageDataAssetDocument;
struct ezImageDataAssetEvent;

class ezQtImageDataAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtImageDataAssetDocumentWindow(ezImageDataAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "ImageDataAsset"; }

private:
  void ImageDataAssetEventHandler(const ezImageDataAssetEvent& e);
  ezEvent<const ezImageDataAssetEvent&>::Unsubscriber m_eventUnsubscriper;

  void UpdatePreview();

  QPointer<ezQtImageWidget> m_pImageWidget;
};

