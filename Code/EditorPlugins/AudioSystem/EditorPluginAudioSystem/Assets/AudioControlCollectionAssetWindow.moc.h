#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtAudioControlCollectionAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtAudioControlCollectionAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtAudioControlCollectionAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AudioControlCollectionAsset"; }
};
