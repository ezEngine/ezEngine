#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Time/Time.h>

#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

/// Play/stop button, playback progress and an info text, for the windows of sound assets.
///
/// This is only the UI, the owner does the actual playback, since that depends on the audio middleware.
/// The owner reacts to PlayRequested() and StopRequested() and reports the result through SetPlaybackState().
/// While the state is 'playing', UpdateRequested() is fired periodically, at which point the owner should call SetPlaybackState() again.
class EZ_EDITORFRAMEWORK_DLL ezQtSoundPreviewWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtSoundPreviewWidget(QWidget* pParent = nullptr);

  /// Text below the playback controls, typically stats about the sound or the reason why it can't be played.
  void SetInfoText(ezStringView sText);

  /// Disables the play button. Has no effect on the button while a sound is playing, so that it can still be stopped.
  void SetCanPlay(bool bCanPlay);

  /// Updates button and progress display.
  ///
  /// A length of zero means the length is unknown, the progress bar then only shows that something is playing.
  void SetPlaybackState(bool bPlaying, ezTime position = ezTime::MakeZero(), ezTime length = ezTime::MakeZero());

  bool IsPlaying() const { return m_bPlaying; }

Q_SIGNALS:
  void PlayRequested();
  void StopRequested();
  void UpdateRequested();

private:
  void UpdateButton();

  QPushButton* m_pPlayButton = nullptr;
  QProgressBar* m_pProgress = nullptr;
  QLabel* m_pTimeLabel = nullptr;
  QLabel* m_pInfoLabel = nullptr;
  QTimer* m_pTimer = nullptr;

  bool m_bPlaying = false;
  bool m_bCanPlay = true;
};
