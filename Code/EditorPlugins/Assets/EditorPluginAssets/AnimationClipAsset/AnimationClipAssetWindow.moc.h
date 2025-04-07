#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Time/Clock.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezQtTimeScrubberWidget;
class ezQtEventTrackEditorWidget;
class ezQtDocumentPanel;
struct ezCommandHistoryEvent;

class ezQtAnimationClipAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtAnimationClipAssetDocumentWindow(ezAnimationClipAssetDocument* pDocument);
  ~ezQtAnimationClipAssetDocumentWindow();

  ezAnimationClipAssetDocument* GetAnimationClipDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationClipAsset"; }

  void ExtractRootMotionFromFeet();

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

  virtual void CommonAssetUiEventHandler(const ezCommonAssetUiState& e) override;
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);

protected Q_SLOTS:
  void OnScrubberPosChangedEvent(ezUInt64 uiNewScrubberTickPos);

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(ezInt64 tickX, QString value);
  void onEventTrackCpMoved(ezUInt32 cpIdx, ezInt64 iTickX);
  void onEventTrackCpDeleted(ezUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose = 0);
  void UpdateEventTrackEditor();

  ezClock m_Clock;
  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  ezQtTimeScrubberWidget* m_pTimeScrubber = nullptr;
  ezTime m_ClipDuration;
  ezTime m_PlaybackPosition;

  ezQtDocumentPanel* m_pEventTrackPanel = nullptr;
  ezQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;
};
