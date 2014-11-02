#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

class EZ_FOUNDATION_DLL ezThreadSignal
{
public:
  ezThreadSignal();
  ~ezThreadSignal();

  /// \brief Waits (and puts the waiting thread to sleep), until the signal is raised.
  void WaitForSignal();

  /// \brief Wakes up one thread that is currently waiting for this signal.
  ///
  /// If no thread is currently waiting for the signal, it stays set, and the next thread that calls 'WaitForSignal' will
  /// will continue uninterrupted.
  /// If more than one thread is waiting for the signal, one of them is awoken (randomly), while the others stay asleep.
  /// The signal is reset immediately after awakening one thread, so it must be signaled again to awaken another thread.
  void RaiseSignal();

private:

  ezThreadSignalData m_Data;
};


