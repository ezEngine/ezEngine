#pragma once

#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezMiniAudioSoundAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezMiniAudioSoundAssetDocumentWindow(ezDocument* pDocument);

  virtual const char* GetGroupName() const { return "MiniAudioSoundAsset"; }
  virtual const char* GetWindowLayoutGroupName() const override { return "MiniAudioSoundAsset"; }

private:
  ezMiniAudioSoundAssetDocument* m_pAssetDoc;
};
