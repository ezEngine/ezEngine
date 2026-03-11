#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

struct ezTime;
class ezThread;

/// \brief Contains general thread functions.
class EZ_FOUNDATION_DLL ezThreadUtils
{
public:
  /// \brief Suspends execution of the current thread and yields the remaining time slice to other threads.
  ///
  /// This allows other threads or processes to run. Use this in spin-wait loops or when the current thread
  /// is waiting for work from other threads. On most platforms this translates to a scheduler yield.
  /// Prefer this over YieldHardwareThread() for general cooperative multitasking scenarios.
  static void YieldTimeSlice();

  /// \brief Yields execution to other hardware threads on the same physical processor core.
  ///
  /// This is a hint to the processor to allow other hardware threads (hyperthreads) on the same core to execute.
  /// Only useful on processors with simultaneous multithreading (SMT/hyperthreading). Does nothing on processors
  /// without hardware threading support. Use this in tight loops where you're waiting for memory operations
  /// or when you want to be more cooperative with hardware threads on the same core without giving up the
  /// time slice to other processes.
  static void YieldHardwareThread();

  /// \brief Suspends the execution of the current thread for the given amount of time.
  ///
  /// The actual sleep duration may be longer than requested due to OS scheduling granularity and system load.
  /// Precision varies by platform but is typically around 1-15ms. For high-precision timing, consider using
  /// busy-wait loops with YieldTimeSlice() for very short delays, though this consumes more CPU.
  /// Avoid using Sleep() in performance-critical code paths.
  static void Sleep(const ezTime& duration); // [tested]

  /// \brief Checks if the current thread is the main thread.
  ///
  /// The main thread is defined as the thread that initialized the Foundation library. This is useful for
  /// assertions and ensuring certain operations only happen on the main thread (e.g., UI operations,
  /// single-threaded subsystem access). Returns true only for the thread that called Foundation startup.
  static bool IsMainThread();

  /// \brief Returns a unique identifier for the currently executing thread.
  ///
  /// The returned ID is guaranteed to be unique among all currently running threads, but may be reused
  /// after a thread terminates. Thread IDs should not be stored long-term or used for cross-process
  /// communication. Primarily useful for debugging, logging, and temporary thread identification.
  static ezThreadID GetCurrentThreadID();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ThreadUtils);

  /// \brief Initialization functionality of the threading system (called by foundation startup and thus private)
  static void Initialize();
};
