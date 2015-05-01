#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>

class QProgressDialog;
class QWinTaskbarProgress;
class QWinTaskbarButton;

/// \brief Handles the display of a modal progress dialog.
///
/// QtProgressBar objects represent one task that consists of a pre-determined number of steps.
/// The objects should be created on the stack, surrounding the code that is doing a blocking operation.
/// Call Update() to advance the progress.
/// Multiple QtProgressBar objects can be used in a nested manner, ie. code that already uses a QtProgressBar
/// can call other code that also uses a QtProgressBar. The nested progress bars will use a sub-range of the
/// parent progress bar.
class EZ_GUIFOUNDATION_DLL QtProgressBar
{
public:
  /// \brief Shows a progress dialog
  ///
  /// \param szMainText The text that is shown in the upper line of the progress dialog label. This is fixed for the life-time of one QtProgressBar object.
  /// \param uiItems The number of subdivisions for this progress range.
  /// \param TimeInvisible Only when the duration takes longer than this, is the progress dialog shown.
  QtProgressBar(const char* szMainText, ezUInt32 uiItems, bool bCanBeCanceled, ezTime TimeInvisible = ezTime::Milliseconds(500));
  ~QtProgressBar();

  /// \brief Advances the progress bar status
  ///
  /// \param szSubText The text that is shown in the lower line of the progress dialog label.
  /// \param uiWorkingOnItems By how many steps to advance the progress bar, ie. how many items we are going to work on next.
  ///        Using a value larger than one is mostly useful when progress bars are nested. This way the nested bar gets a larger piece
  ///        of the parent range.
  static void WorkingOnNextItem(const char* szSubText = NULL, ezUInt32 uiWorkingOnItems = 1);

  /// \brief Whether the user clicked the cancel button.
  static bool WasProgressBarCanceled();

  struct Event
  {
    enum class Type
    {
      ProgressStarted,  ///< Sent when the progress dialog is shown.
      ProgressEnded,    ///< Sent when the progress dialog closes, e.g. when all progress is finished.
      ProgressChanged,  ///< Sent whenever the progress value changes. Not necessarily in every update step.
    };

    Type m_Type;
    ezUInt32 m_uiProMilleValue;
  };

  /// \brief These events can be used to react to the blocking progress display, e.g. to disable continuous rendering
  static ezEvent<const Event&> s_Events;

private:
  static QProgressDialog* s_pQtProgressBar;
  static QtProgressBar* s_pActiveRange;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  static QWinTaskbarButton* s_pWinTaskBarButton;
  static QWinTaskbarProgress* s_pWinTaskBarProgress;
#endif

  void InternalUpdate(const char* szSubText, ezUInt32 uiWorkingOnItems);
  void UpdateDisplay();

  ezString m_sCurrentMainText;
  ezString m_sCurrentSubText;

  bool m_bCanBeCanceled;
  QtProgressBar* m_pParentRange;
  double m_dPercentageRange;
  double m_dBaseValue;
  ezUInt32 m_uiMaxItems;
  ezUInt32 m_uiCurItems;
  ezUInt32 m_uiWorkingOnItems;
};


