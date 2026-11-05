#pragma once

#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <Foundation/Basics.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtSoundPreviewWidget;
class ezSoundBankPreview;
class QTableWidget;

class ezSoundBankAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezSoundBankAssetDocumentWindow(ezDocument* pDocument);
  ~ezSoundBankAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "SoundBankAsset"; }

private:
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void SubAssetChosenHandler(const ezUuid& subAssetGuid);
  void ReloadBank();
  void UpdateInfoText();
  void Play(ezUInt32 uiEventIndex);

  void OnPlayRequested();
  void OnStopRequested();
  void OnUpdateRequested();

  ezSoundBankAssetDocument* m_pAssetDoc;
  QTableWidget* m_pEventTable = nullptr;
  ezQtSoundPreviewWidget* m_pPreviewWidget = nullptr;
  ezUniquePtr<ezSoundBankPreview> m_pPreview;
  ezTime m_PlayingLength;
  ezStatus m_LoadStatus = ezStatus(EZ_SUCCESS);
};
