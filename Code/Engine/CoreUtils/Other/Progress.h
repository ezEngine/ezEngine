#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>

class ezProgress;
class ezProgressRange;

struct EZ_COREUTILS_DLL ezProgressEvent
{
  enum class Type
  {
    ProgressStarted,  ///< Sent when the the first progress starts
    ProgressEnded,    ///< Sent when progress finishes or is canceled
    ProgressChanged,  ///< Sent whenever the progress value changes. Not necessarily in every update step.
    CancelClicked,
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
class EZ_COREUTILS_DLL ezProgress
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


  static ezProgress* GetGlobalProgressbar();

  static void SetGlobalProgressbar(ezProgress* pProgress);

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

class EZ_COREUTILS_DLL ezProgressRange
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProgressRange);

public:
  ezProgressRange(const char* szDisplayText, ezUInt32 uiSteps, bool bAllowCancel, ezProgress* pProgressbar = nullptr);
  ~ezProgressRange();

  ezProgress* GetProgressbar() const;

  void SetStepWeighting(ezUInt32 uiStep, float fWeigth);

  void BeginNextStep(const char* szStepDisplayText, ezUInt32 uiNumSteps = 1);

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

