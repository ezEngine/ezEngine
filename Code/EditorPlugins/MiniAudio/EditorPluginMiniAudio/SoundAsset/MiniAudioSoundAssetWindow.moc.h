#pragma once

#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <Foundation/Basics.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezMiniAudioSoundPreview;
class ezQtSoundPreviewWidget;

class ezMiniAudioSoundAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezMiniAudioSoundAssetDocumentWindow(ezDocument* pDocument);
  ~ezMiniAudioSoundAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "MiniAudioSoundAsset"; }

private:
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void UpdateFileInfo();

  void OnPlayRequested();
  void OnStopRequested();
  void OnUpdateRequested();

  ezMiniAudioSoundAssetDocument* m_pAssetDoc;
  ezQtSoundPreviewWidget* m_pPreviewWidget = nullptr;
  ezUniquePtr<ezMiniAudioSoundPreview> m_pPreview;
  ezRandom m_Rng;

  struct PlayableFile
  {
    ezString m_sAbsolutePath;
    ezTime m_Length;
  };

  /// Filled by UpdateFileInfo(), because determining the length of a file can be slow.
  ezHybridArray<PlayableFile, 4> m_PlayableFiles;
};
