#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

class EZ_FOUNDATION_DLL ezThreadUtils
{
public:

  // Suspends execution of the current thread
  static void YieldTimeSlice();

  // Suspends the execution of the current thread for the given amount of time. (Precision may vary according to OS)
  static void Sleep(ezUInt32 uiMilliSeconds);

  // Returns the current thread handle
  static ezThreadHandle GetCurrentThreadHandle();

  // Helper function to check if the current thread is the main thread (e.g. the thread which initialized the foundation library)
  static bool IsMainThread();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ThreadUtils);

  // Initialization functionality of the threading system (called by foundation startup and thus private)
  static void Initialize();

  // Cleanup functionality of the threading system (called by foundation shutdown and thus private)
  static void Shutdown();
};

