#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/ConditionVariable.h>

/// \brief Waiting on a thread signal puts the waiting thread to sleep. Other threads can wake it up by raising the signal.
///
/// ezThreadSignal is similar to ezConditionVariable but adds some internal state, which makes it more suitable for common use cases.
/// For instance, in contrast to ezConditionVariable, one can wait for an ezThreadSignal and get awoken, even if the signal was raised
/// before a thread tried to wait on it.
/// At any given time the thread signal is either 'raised' or 'cleared'. Waiting for a 'raised' signal will return immediately.
/// This makes it easier to implement a simple producer/consumer scenario.
///
/// Once a waiting thread is woken up, the signal state is cleared automatically, unless the ezThreadSignal uses 'ManualReset' mode.
/// Thus, in AutoReset mode, it is guaranteed that exactly one thread will be woken up (or not even put to sleep) for every signal.
///
/// If an already raised ezThreadSignal is raised again, this has no effect.
class EZ_FOUNDATION_DLL ezThreadSignal
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezThreadSignal);

public:
  enum class Mode
  {
    AutoReset,  ///< In this mode the signal is automatically reset once a thread has finished waiting for it
    ManualReset ///< In this mode the signal stays set until it is manually reset and all waiting threads may get awaken.
  };

  enum class WaitResult
  {
    Signaled,
    Timeout
  };

  ezThreadSignal(Mode mode = Mode::AutoReset);
  ~ezThreadSignal();

  /// \brief Waits until the signal is raised.
  ///
  /// The waiting thread is put to sleep in the mean time.
  void WaitForSignal() const;

  /// \brief Waits until either the signal is raised or the timeout is reached.
  ///
  /// The waiting thread is put to sleep in the mean time.
  WaitResult WaitForSignal(ezTime timeout) const;

  /// \brief Wakes up one thread that is currently waiting for this signal.
  ///
  /// If no thread is currently waiting for the signal, it stays set, and the next thread that calls 'WaitForSignal'
  /// will continue uninterrupted.
  ///
  /// In AutoReset mode, if more than one thread is waiting for the signal, one of them is awoken (randomly), while the others stay asleep.
  /// The signal is reset immediately after awakening one thread, so it must be signaled again to awaken another thread.
  ///
  /// In ManualReset mode all threads that called WaitForSignal may get awoken. All future threads calling WaitForSignal
  /// will immediately continue and not even got to sleep. Only once ClearSignal() is executed, will threads be put to sleep again.
  void RaiseSignal();

  /// \brief Mostly relevant for ManualReset mode, to reset the signal state.
  void ClearSignal();

private:
  Mode m_mode = Mode::AutoReset;
  mutable bool m_bSignalState = false;
  mutable ezConditionVariable m_ConditionVariable;
};
