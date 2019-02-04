#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>

class ezProgress;
class ezProgressRange;

/// \brief Through these events the state of an ezProgress instance is communicated.
///
/// Other code can use this to visualize the progress in different ways.
/// For instance a GUI application can show a progress bar dialog and a game
/// could show a loading screen.
struct EZ_FOUNDATION_DLL ezProgressEvent
{
  enum class Type
  {
    ProgressStarted, ///< Sent when the the first progress starts
    ProgressEnded,   ///< Sent when progress finishes or is canceled
    ProgressChanged, ///< Sent whenever the progress value changes. Not necessarily in every update step.
    CancelClicked,   ///< The user just clicked cancel (for the first time).
  };

  Type m_Type;
  ezProgress* m_pProgressbar;
};

/// \brief Manages the way a progress bar is subdivided and advanced.
///
/// ezProgress represents a single progress bar. It can be sub-divided into groups and sub-groups using ezProgressbarRange.
/// From the ranges and the current advancement, a final progress percentage is computed. Every time a significant change
/// takes place, an event is broadcast. This allows other code to display the progress, either in a GUI application
/// or in a fullscreen loading screen or in any other way appropriate.
class EZ_FOUNDATION_DLL ezProgress
{
public:
  ezProgress();
  ~ezProgress();

  /// \brief Returns the current overall progress in [0; 1] range.
  float GetCompletion() const;

  /// \brief Sets the current overall progress in [0; 1] range. Should not be called directly, typically called by ezProgreesRange.
  void SetCompletion(float fCompletion);

  /// \brief Returns the current 'headline' text for the progress bar
  const char* GetMainDisplayText() const;

  /// \brief Returns the current detail text for the progress bar
  const char* GetStepDisplayText() const;

  /// \brief Used to inform ezProgress of outside user input. May have an effect or not.
  void UserClickedCancel();

  /// \brief Returns whether the current operations may be canceled or not.
  bool AllowUserCancel() const;

  /// \brief Returns the currently set default ezProgress instance. This will always be valid.
  static ezProgress* GetGlobalProgressbar();

  /// \brief Allows to set a custom ezProgress instance as the global default instance.
  static void SetGlobalProgressbar(ezProgress* pProgress);

  /// Events are sent when the progress changes
  ezEvent<const ezProgressEvent&> m_Events;

private:
  friend class ezProgressRange;
  void SetActiveRange(ezProgressRange* pRange);

  ezProgressRange* m_pActiveRange;

  ezString m_sCurrentDisplayText;
  bool m_bCancelClicked;
  bool m_bEnableCancel;

  float m_fLastReportedCompletion;
  float m_fCurrentCompletion;
};

/// \brief ezProgressRange is the preferred method to inform the system of progress.
///
/// ezProgressRange is a scoped class, ie. upon creation it adds a range to the current progress
/// and upon destruction the entire range is considered to be completed.
/// Ranges can be nested. For instance when a top level range consists of three 'steps',
/// then opening a nested range will sub-divide that first step. When the nested range is closed,
/// the first top-level step is finished and BeginNextStep() should be called on the top-level range.
/// Subsequently the second step is active and can again be further subdivided with another nested ezProgressRange.
class EZ_FOUNDATION_DLL ezProgressRange
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProgressRange);

public:
  /// \brief Creates a progress range scope.
  ///
  /// If any other progress range is currently active, it will become the parent range and the currently active step will be subdivided.
  /// \param szDisplayText is the main display text for this range.
  /// \param uiSteps is the number of steps that this range will be subdivided into
  /// \param bAllowCancel specifies whether the user can cancel this operation
  /// \param pProgressbar can be specified, if available, otherwise the currently active ezProgress instance is used.
  ezProgressRange(const char* szDisplayText, ezUInt32 uiSteps, bool bAllowCancel, ezProgress* pProgressbar = nullptr);

  /// \brief The destructor closes the current range. All progress in this range is assumed to have completed,
  /// even if BeginNextStep() has not been called once for every subdivision step.
  ~ezProgressRange();

  /// \brief Returns the ezProgress instance that this range uses.
  ezProgress* GetProgressbar() const;

  /// \brief Allows to weigh each step differently.
  ///
  /// This makes it possible to divide an operation into two steps, but have one part take up 90% and the other 10%.
  /// \param uiStep The index for the step to set the weight
  /// \param fWeigth The weighting in [0; 1] range
  void SetStepWeighting(ezUInt32 uiStep, float fWeigth);

  /// \brief Should be called whenever a new sub-step is started to advance the progress.
  ///
  /// \param szStepDisplayText The sub-text for the next step to be displayed.
  /// \param uiNumSteps How many steps have been completed.
  void BeginNextStep(const char* szStepDisplayText, ezUInt32 uiNumSteps = 1);

  /// \brief Whether the user requested to cancel the operation.
  bool WasCanceled() const;

private:
  friend class ezProgress;

  void ComputeCurStepBaseAndRange(double& out_base, double& out_range);
  double ComputeInternalCompletion() const;

  ezProgressRange* m_pParentRange;
  ezProgress* m_pProgressbar;

  ezUInt32 m_uiCurrentStep;
  ezString m_sDisplayText;
  ezString m_sStepDisplayText;
  ezHybridArray<float, 16> m_StepWeights;

  bool m_bAllowCancel;
  double m_PercentageBase;
  double m_PercentageRange;
  double m_SummedWeight;
};

