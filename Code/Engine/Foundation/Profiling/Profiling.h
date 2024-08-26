#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Time.h>

class ezStreamWriter;
class ezThread;

/// \brief This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE_SCOPE provided below.
class EZ_FOUNDATION_DLL ezProfilingScope
{
public:
  ezProfilingScope(ezStringView sName, const char* szFunctionName, ezTime timeout);
  ~ezProfilingScope();

protected:
  ezStringView m_sName;
  const char* m_szFunction;
  ezTime m_BeginTime;
  ezTime m_Timeout;
};

/// \brief This class implements a profiling scope similar to ezProfilingScope, but with additional sub-scopes which can be added easily without
/// introducing actual C++ scopes.
///
/// The constructor pushes one surrounding scope on the stack and then a nested scope as the first section.
/// The function StartNextSection() will end the nested scope and start a new inner scope.
/// This allows to end one scope and start a new one, without having to add actual C++ scopes for starting/stopping profiling scopes.
///
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE_LIST_SCOPE provided below.
class ezProfilingListScope
{
public:
  EZ_FOUNDATION_DLL ezProfilingListScope(ezStringView sListName, ezStringView sFirstSectionName, const char* szFunctionName);
  EZ_FOUNDATION_DLL ~ezProfilingListScope();

  EZ_FOUNDATION_DLL static void StartNextSection(ezStringView sNextSectionName);

protected:
  static thread_local ezProfilingListScope* s_pCurrentList;

  ezProfilingListScope* m_pPreviousList;

  ezStringView m_sListName;
  const char* m_szListFunction;
  ezTime m_ListBeginTime;

  ezStringView m_sCurSectionName;
  ezTime m_CurSectionBeginTime;
};

/// \brief Helper functionality of the profiling system.
class EZ_FOUNDATION_DLL ezProfilingSystem
{
public:
  struct ThreadInfo
  {
    ezUInt64 m_uiThreadId;
    ezString m_sName;
  };

  struct CPUScope
  {
    EZ_DECLARE_POD_TYPE();

    static constexpr ezUInt32 NAME_SIZE = 40;

    const char* m_szFunctionName;
    ezTime m_BeginTime;
    ezTime m_EndTime;
    char m_szName[NAME_SIZE];
  };

  struct CPUScopesBufferFlat
  {
    ezDynamicArray<CPUScope> m_Data;
    ezUInt64 m_uiThreadId = 0;
  };

  /// \brief Helper struct to hold GPU profiling data.
  struct GPUScope
  {
    EZ_DECLARE_POD_TYPE();

    static constexpr ezUInt32 NAME_SIZE = 48;

    ezTime m_BeginTime;
    ezTime m_EndTime;
    char m_szName[NAME_SIZE];
  };

  struct EZ_FOUNDATION_DLL ProfilingData
  {
    ezUInt32 m_uiFramesThreadID = 0;
    ezUInt32 m_uiProcessSortIndex = 0;
    ezOsProcessID m_uiProcessID = 0;

    ezHybridArray<ThreadInfo, 16> m_ThreadInfos;

    ezDynamicArray<CPUScopesBufferFlat> m_AllEventBuffers;

    ezUInt64 m_uiFrameCount = 0;
    ezDynamicArray<ezTime> m_FrameStartTimes;

    ezDynamicArray<ezDynamicArray<GPUScope>> m_GPUScopes;

    /// \brief Writes profiling data as JSON to the output stream.
    ezResult Write(ezStreamWriter& ref_outputStream) const;

    void Clear();

    /// \brief Concatenates all given ProfilingData instances into one merge struct
    static void Merge(ProfilingData& out_merged, ezArrayPtr<const ProfilingData*> inputs);
  };

public:
  static void Clear();

  static void Capture(ezProfilingSystem::ProfilingData& out_capture, bool bClearAfterCapture = false);

  /// \brief Scopes are discarded if their duration is shorter than the specified threshold. Default is 0.1ms.
  static void SetDiscardThreshold(ezTime threshold);

  using ScopeTimeoutDelegate = ezDelegate<void(ezStringView sName, ezStringView sFunctionName, ezTime duration)>;

  /// \brief Sets a callback that is triggered when a profiling scope takes longer than desired.
  static void SetScopeTimeoutCallback(ScopeTimeoutDelegate callback);

  /// \brief Should be called once per frame to capture the timestamp of the new frame.
  static void StartNewFrame();

  /// \brief Adds a new scoped event for the calling thread in the profiling system
  static void AddCPUScope(ezStringView sName, const char* szFunctionName, ezTime beginTime, ezTime endTime, ezTime scopeTimeout);

  /// \brief Get current frame counter
  static ezUInt64 GetFrameCount();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend ezUInt32 RunThread(ezThread* pThread);

  static void Initialize();
  /// \brief Removes profiling data of dead threads.
  static void Reset();

  /// \brief Sets the name of the current thread.
  static void SetThreadName(ezStringView sThreadName);
  /// \brief Removes the current thread from the profiling system.
  ///  Needs to be called before the thread exits to be able to release profiling memory of dead threads on Reset.
  static void RemoveThread();

public:
  /// \brief Initialized internal data structures for GPU profiling data. Needs to be called before adding any data.
  static void InitializeGPUData(ezUInt32 uiGpuCount = 1);

  /// \brief Adds a GPU profiling scope in the internal event ringbuffer.
  static void AddGPUScope(ezStringView sName, ezTime beginTime, ezTime endTime, ezUInt32 uiGpuIndex = 0);
};

#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)

/// \brief Profiles the current scope using the given name.
///
/// It is allowed to nest EZ_PROFILE_SCOPE, also with EZ_PROFILE_LIST_SCOPE. However EZ_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa ezProfilingScope
/// \sa EZ_PROFILE_LIST_SCOPE
#  define EZ_PROFILE_SCOPE(ScopeName) \
    ezProfilingScope EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(ScopeName, EZ_SOURCE_FUNCTION, ezTime::MakeZero())

/// \brief Same as EZ_PROFILE_SCOPE but if the scope takes longer than 'Timeout', the ezProfilingSystem's timeout callback is executed.
///
/// This can be used to log an error or save a callstack, etc. when a scope exceeds an expected amount of time.
///
/// \sa ezProfilingSystem::SetScopeTimeoutCallback()
#  define EZ_PROFILE_SCOPE_WITH_TIMEOUT(ScopeName, Timeout) \
    ezProfilingScope EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(ScopeName, EZ_SOURCE_FUNCTION, Timeout)

/// \brief Profiles the current scope using the given name as the overall list scope name and the section name for the first section in the list.
///
/// Use EZ_PROFILE_LIST_NEXT_SECTION to start a new section in the list scope.
///
/// It is allowed to nest EZ_PROFILE_SCOPE, also with EZ_PROFILE_LIST_SCOPE. However EZ_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa ezProfilingListScope
/// \sa EZ_PROFILE_LIST_NEXT_SECTION
#  define EZ_PROFILE_LIST_SCOPE(ListName, FirstSectionName) \
    ezProfilingListScope EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(ListName, FirstSectionName, EZ_SOURCE_FUNCTION)

/// \brief Starts a new section in a EZ_PROFILE_LIST_SCOPE
///
/// \sa ezProfilingListScope
/// \sa EZ_PROFILE_LIST_SCOPE
#  define EZ_PROFILE_LIST_NEXT_SECTION(NextSectionName) \
    ezProfilingListScope::StartNextSection(NextSectionName)

/// \brief Used to indicate that a frame is finished and another starts.
#  define EZ_PROFILER_FRAME_MARKER()

#else
#  define EZ_PROFILE_SCOPE(ScopeName)
#  define EZ_PROFILE_SCOPE_WITH_TIMEOUT(ScopeName, Timeout)
#  define EZ_PROFILE_LIST_SCOPE(ListName, FirstSectionName)
#  define EZ_PROFILE_LIST_NEXT_SECTION(NextSectionName)
#  define EZ_PROFILER_FRAME_MARKER()
#endif

// Let Tracy override the macros.
#include <Foundation/Profiling/Profiling_Tracy.h>
