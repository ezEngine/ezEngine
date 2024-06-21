#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/DataTransfer.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadUtils.h>

#if EZ_ENABLED(EZ_USE_PROFILING)

class ezProfileCaptureDataTransfer : public ezDataTransfer
{
private:
  virtual void OnTransferRequest() override
  {
    ezDataTransferObject dto(*this, "Capture", "application/json", "json");

    ezProfilingSystem::ProfilingData profilingData;
    ezProfilingSystem::Capture(profilingData);
    profilingData.Write(dto.GetWriter()).IgnoreResult();

    dto.Transmit();
  }
};

static ezProfileCaptureDataTransfer s_ProfileCaptureDataTransfer;

namespace
{
  static ezEventSubscriptionID s_PluginEventSubscription = 0;
  void PluginEvent(const ezPluginEvent& e)
  {
    if (e.m_EventType == ezPluginEvent::AfterUnloading)
    {
      // When a plugin is unloaded we need to clear all profiling data
      // since they can contain pointers to function names that don't exist anymore.
      ezProfilingSystem::Clear();
    }
  }
} // namespace

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    ezProfilingSystem::Initialize();
  }
  ON_CORESYSTEMS_STARTUP
  { 
    s_PluginEventSubscription = ezPlugin::Events().AddEventHandler(&PluginEvent);
    s_ProfileCaptureDataTransfer.EnableDataTransfer("Profiling Capture");
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_ProfileCaptureDataTransfer.DisableDataTransfer();
    ezPlugin::Events().RemoveEventHandler(s_PluginEventSubscription);
    ezProfilingSystem::Reset();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  enum
  {
    BUFFER_SIZE_OTHER_THREAD = 1024 * 1024,
    BUFFER_SIZE_MAIN_THREAD = BUFFER_SIZE_OTHER_THREAD * 4 ///< Typically the main thread allocated a lot more profiling events than other threads
  };

  enum
  {
    BUFFER_SIZE_FRAMES = 120 * 60,
  };

  using GPUScopesBuffer = ezStaticRingBuffer<ezProfilingSystem::GPUScope, BUFFER_SIZE_OTHER_THREAD / sizeof(ezProfilingSystem::GPUScope)>;

  static ezUInt64 s_MainThreadId = 0;

  struct CpuScopesBufferBase
  {
    virtual ~CpuScopesBufferBase() = default;

    ezUInt64 m_uiThreadId = 0;
    bool IsMainThread() const { return m_uiThreadId == s_MainThreadId; }
  };

  template <ezUInt32 SizeInBytes>
  struct CpuScopesBuffer : public CpuScopesBufferBase
  {
    ezStaticRingBuffer<ezProfilingSystem::CPUScope, SizeInBytes / sizeof(ezProfilingSystem::CPUScope)> m_Data;
  };

  CpuScopesBuffer<BUFFER_SIZE_MAIN_THREAD>* CastToMainThreadEventBuffer(CpuScopesBufferBase* pEventBuffer)
  {
    EZ_ASSERT_DEV(pEventBuffer->IsMainThread(), "Implementation error");
    return static_cast<CpuScopesBuffer<BUFFER_SIZE_MAIN_THREAD>*>(pEventBuffer);
  }

  CpuScopesBuffer<BUFFER_SIZE_OTHER_THREAD>* CastToOtherThreadEventBuffer(CpuScopesBufferBase* pEventBuffer)
  {
    EZ_ASSERT_DEV(!pEventBuffer->IsMainThread(), "Implementation error");
    return static_cast<CpuScopesBuffer<BUFFER_SIZE_OTHER_THREAD>*>(pEventBuffer);
  }

  ezCVarFloat cvar_ProfilingDiscardThresholdMS("Profiling.DiscardThresholdMS", 0.1f, ezCVarFlags::Default, "Discard profiling scopes if their duration is shorter than this in milliseconds.");

  ezStaticRingBuffer<ezTime, BUFFER_SIZE_FRAMES> s_FrameStartTimes;
  ezUInt64 s_uiFrameCount = 0;

  static ezHybridArray<ezProfilingSystem::ThreadInfo, 16> s_ThreadInfos;
  static ezHybridArray<ezUInt64, 16> s_DeadThreadIDs;
  static ezMutex s_ThreadInfosMutex;

#  if EZ_ENABLED(EZ_PLATFORM_64BIT)
  static_assert(sizeof(ezProfilingSystem::CPUScope) == 64);
  static_assert(sizeof(ezProfilingSystem::GPUScope) == 64);
#  endif

  static thread_local CpuScopesBufferBase* s_CpuScopes = nullptr;
  static ezDynamicArray<CpuScopesBufferBase*> s_AllCpuScopes;
  static ezMutex s_AllCpuScopesMutex;
  static ezProfilingSystem::ScopeTimeoutDelegate s_ScopeTimeoutCallback;

  static ezDynamicArray<ezUniquePtr<GPUScopesBuffer>> s_GPUScopes;
} // namespace

void ezProfilingSystem::ProfilingData::Clear()
{
  m_uiFramesThreadID = 0;
  m_uiProcessID = 0;
  m_uiFrameCount = 0;

  m_AllEventBuffers.Clear();
  m_FrameStartTimes.Clear();
  m_GPUScopes.Clear();
  m_ThreadInfos.Clear();
}

void ezProfilingSystem::ProfilingData::Merge(ProfilingData& out_merged, ezArrayPtr<const ProfilingData*> inputs)
{
  out_merged.Clear();

  if (inputs.IsEmpty())
    return;

  out_merged.m_uiProcessID = inputs[0]->m_uiProcessID;
  out_merged.m_uiFramesThreadID = inputs[0]->m_uiFramesThreadID;

  // concatenate m_FrameStartTimes and m_GPUScopes and m_uiFrameCount
  {
    ezUInt32 uiNumFrameStartTimes = 0;
    ezUInt32 uiNumGpuScopes = 0;

    for (const auto& pd : inputs)
    {
      out_merged.m_uiFrameCount += pd->m_uiFrameCount;

      uiNumFrameStartTimes += pd->m_FrameStartTimes.GetCount();
      uiNumGpuScopes += pd->m_GPUScopes.GetCount();
    }

    out_merged.m_FrameStartTimes.Reserve(uiNumFrameStartTimes);
    out_merged.m_GPUScopes.Reserve(uiNumGpuScopes);

    for (const auto& pd : inputs)
    {
      out_merged.m_FrameStartTimes.PushBackRange(pd->m_FrameStartTimes);
      out_merged.m_GPUScopes.PushBackRange(pd->m_GPUScopes);
    }
  }

  // merge m_ThreadInfos
  {
    auto threadInfoAlreadyKnown = [out_merged](ezUInt64 uiThreadId) -> bool
    {
      for (const auto& ti : out_merged.m_ThreadInfos)
      {
        if (ti.m_uiThreadId == uiThreadId)
          return true;
      }

      return false;
    };

    for (const auto& pd : inputs)
    {
      for (const auto& ti : pd->m_ThreadInfos)
      {
        if (!threadInfoAlreadyKnown(ti.m_uiThreadId))
        {
          out_merged.m_ThreadInfos.PushBack(ti);
        }
      }
    }
  }

  // merge m_AllEventBuffers
  {
    struct CountAndIndex
    {
      ezUInt32 m_uiCount = 0;
      ezUInt32 m_uiIndex = 0xFFFFFFFF;
    };

    ezMap<ezUInt64, CountAndIndex> eventBufferInfos;

    // gather info about required size of the output array
    for (const auto& pd : inputs)
    {
      for (const auto& eb : pd->m_AllEventBuffers)
      {
        auto& ebInfo = eventBufferInfos[eb.m_uiThreadId];

        ebInfo.m_uiIndex = ezMath::Min(ebInfo.m_uiIndex, eventBufferInfos.GetCount() - 1);
        ebInfo.m_uiCount += eb.m_Data.GetCount();
      }
    }

    // reserve the output array
    {
      out_merged.m_AllEventBuffers.SetCount(eventBufferInfos.GetCount());

      for (auto ebinfoIt : eventBufferInfos)
      {
        auto& neb = out_merged.m_AllEventBuffers[ebinfoIt.Value().m_uiIndex];
        neb.m_uiThreadId = ebinfoIt.Key();
        neb.m_Data.Reserve(ebinfoIt.Value().m_uiCount);
      }
    }

    // fill the output array
    for (const auto& pd : inputs)
    {
      for (const auto& eb : pd->m_AllEventBuffers)
      {
        const auto& ebInfo = eventBufferInfos[eb.m_uiThreadId];

        out_merged.m_AllEventBuffers[ebInfo.m_uiIndex].m_Data.PushBackRange(eb.m_Data);
      }
    }
  }
}

ezResult ezProfilingSystem::ProfilingData::Write(ezStreamWriter& ref_outputStream) const
{
  ezStandardJSONWriter writer;
  writer.SetWhitespaceMode(ezJSONWriter::WhitespaceMode::None);
  writer.SetOutputStream(&ref_outputStream);

  writer.BeginObject();
  {
    writer.BeginArray("traceEvents");

    // Process metadata
    {
      ezApplication::GetApplicationInstance()->GetApplicationName();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "process_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", ezApplication::GetApplicationInstance() ? ezApplication::GetApplicationInstance()->GetApplicationName().GetData() : "ezEngine");
        writer.EndObject();
      }
      writer.EndObject();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "process_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", m_uiProcessSortIndex);
        writer.EndObject();
      }
      writer.EndObject();
    }

    // Frames thread metadata
    {
      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", "Frames");
        writer.EndObject();
      }
      writer.EndObject();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", -1);
        writer.EndObject();
      }
      writer.EndObject();

      if (writer.HadWriteError())
      {
        return EZ_FAILURE;
      }
    }

    const ezUInt32 uiGpuCount = m_GPUScopes.GetCount();
    // GPU thread metadata
    // Since there are no actual threads, we assign 1..uiGpuCount as the respective threadID
    for (ezUInt32 gpuIndex = 1; gpuIndex <= uiGpuCount; ++gpuIndex)
    {
      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", gpuIndex);
        writer.AddVariableString("ph", "M");

        ezStringBuilder gpuNameBuilder;
        gpuNameBuilder.AppendFormat("GPU {}", gpuIndex - 1);

        writer.BeginObject("args");
        writer.AddVariableString("name", gpuNameBuilder);
        writer.EndObject();
      }
      writer.EndObject();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", gpuIndex);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", -2);
        writer.EndObject();
      }
      writer.EndObject();
      if (writer.HadWriteError())
      {
        return EZ_FAILURE;
      }
    }

    // thread metadata
    {
      for (ezUInt32 threadIndex = 0; threadIndex < m_ThreadInfos.GetCount(); ++threadIndex)
      {
        const ThreadInfo& info = m_ThreadInfos[threadIndex];
        writer.BeginObject();
        {
          writer.AddVariableString("name", "thread_name");
          writer.AddVariableString("cat", "__metadata");
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", info.m_uiThreadId + uiGpuCount + 1);
          writer.AddVariableString("ph", "M");

          writer.BeginObject("args");
          writer.AddVariableString("name", info.m_sName);
          writer.EndObject();
        }
        writer.EndObject();

        writer.BeginObject();
        {
          writer.AddVariableString("name", "thread_sort_index");
          writer.AddVariableString("cat", "__metadata");
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", info.m_uiThreadId + uiGpuCount + 1);
          writer.AddVariableString("ph", "M");

          writer.BeginObject("args");
          writer.AddVariableInt32("sort_index", threadIndex);
          writer.EndObject();
        }
        writer.EndObject();

        if (writer.HadWriteError())
        {
          return EZ_FAILURE;
        }
      }
    }

    // scoped events
    ezDynamicArray<CPUScope> sortedScopes;
    for (const auto& eventBuffer : m_AllEventBuffers)
    {
      // Since we introduced fake thread IDs via the GPUs, we simply shift all real thread IDs to be in a different range to avoid collisions.
      const ezUInt64 uiThreadId = eventBuffer.m_uiThreadId + uiGpuCount + 1;

      // It seems that chrome does a stable sort by scope begin time. Now that we write complete scopes at the end of a scope
      // we actually write nested scopes before their corresponding parent scope to the file. If both start at the same quantized time stamp
      // chrome prints the nested scope first and then scrambles everything.
      // So we sort by duration to make sure that parent scopes are written first in the json file.
      sortedScopes = eventBuffer.m_Data;
      sortedScopes.Sort([](const CPUScope& a, const CPUScope& b)
        { return (a.m_EndTime - a.m_BeginTime) > (b.m_EndTime - b.m_BeginTime); });

      for (const CPUScope& e : sortedScopes)
      {
        writer.BeginObject();
        writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", uiThreadId);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_BeginTime.GetMicroseconds()));
        writer.AddVariableString("ph", "B");

        if (e.m_szFunctionName != nullptr)
        {
          writer.BeginObject("args");
          writer.AddVariableString("function", e.m_szFunctionName);
          writer.EndObject();
        }

        writer.EndObject();

        if (e.m_EndTime.IsPositive())
        {
          writer.BeginObject();
          writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", uiThreadId);
          writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_EndTime.GetMicroseconds()));
          writer.AddVariableString("ph", "E");
          writer.EndObject();
        }

        if (writer.HadWriteError())
        {
          return EZ_FAILURE;
        }
      }
    }

    // frame start/end
    {
      ezStringBuilder sFrameName;

      const ezUInt32 uiNumFrames = m_FrameStartTimes.GetCount();
      for (ezUInt32 i = 1; i < uiNumFrames; ++i)
      {
        const ezTime t0 = m_FrameStartTimes[i - 1];
        const ezTime t1 = m_FrameStartTimes[i];

        const ezUInt64 localFrameID = uiNumFrames - i - 1;
        sFrameName.SetFormat("Frame {}", m_uiFrameCount - localFrameID);

        writer.BeginObject();
        writer.AddVariableString("name", sFrameName);
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(t0.GetMicroseconds()));
        writer.AddVariableString("ph", "B");
        writer.EndObject();

        writer.BeginObject();
        writer.AddVariableString("name", sFrameName);
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(t1.GetMicroseconds()));
        writer.AddVariableString("ph", "E");
        writer.EndObject();
        if (writer.HadWriteError())
        {
          return EZ_FAILURE;
        }
      }
    }

    // GPU data
    // Since there are no actual threads, we assign 1..gpuCount as the respective threadID
    {
      // See comment on sortedScopes above.
      ezDynamicArray<GPUScope> sortedGpuScopes;
      for (ezUInt32 gpuIndex = 1; gpuIndex <= m_GPUScopes.GetCount(); ++gpuIndex)
      {
        sortedGpuScopes = m_GPUScopes[gpuIndex - 1];
        sortedGpuScopes.Sort([](const GPUScope& a, const GPUScope& b)
          { return (a.m_EndTime - a.m_BeginTime) > (b.m_EndTime - b.m_BeginTime); });

        for (ezUInt32 i = 0; i < sortedGpuScopes.GetCount(); ++i)
        {
          const auto& e = sortedGpuScopes[i];

          writer.BeginObject();
          writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", gpuIndex);
          writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_BeginTime.GetMicroseconds()));
          writer.AddVariableString("ph", "B");
          writer.EndObject();

          writer.BeginObject();
          writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", gpuIndex);
          writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_EndTime.GetMicroseconds()));
          writer.AddVariableString("ph", "E");
          writer.EndObject();
          if (writer.HadWriteError())
          {
            return EZ_FAILURE;
          }
        }
      }
    }

    writer.EndArray();
  }

  writer.EndObject();

  return writer.HadWriteError() ? EZ_FAILURE : EZ_SUCCESS;
}

// static
void ezProfilingSystem::Clear()
{
  {
    EZ_LOCK(s_AllCpuScopesMutex);
    for (auto pEventBuffer : s_AllCpuScopes)
    {
      if (pEventBuffer->IsMainThread())
      {
        CastToMainThreadEventBuffer(pEventBuffer)->m_Data.Clear();
      }
      else
      {
        CastToOtherThreadEventBuffer(pEventBuffer)->m_Data.Clear();
      }
    }
  }

  s_FrameStartTimes.Clear();

  for (auto& gpuScopes : s_GPUScopes)
  {
    if (gpuScopes != nullptr)
    {
      gpuScopes->Clear();
    }
  }
}

// static
void ezProfilingSystem::Capture(ezProfilingSystem::ProfilingData& ref_profilingData, bool bClearAfterCapture)
{
  ref_profilingData.Clear();

  ref_profilingData.m_uiFramesThreadID = 0;
#  if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  ref_profilingData.m_uiProcessID = ezProcess::GetCurrentProcessID();
#  else
  ref_profilingData.m_uiProcessID = 0;
#  endif

  {
    EZ_LOCK(s_ThreadInfosMutex);

    if (bClearAfterCapture)
    {
      ref_profilingData.m_ThreadInfos = std::move(s_ThreadInfos);
    }
    else
    {
      ref_profilingData.m_ThreadInfos = s_ThreadInfos;
    }
  }

  {
    EZ_LOCK(s_AllCpuScopesMutex);

    ref_profilingData.m_AllEventBuffers.Reserve(s_AllCpuScopes.GetCount());
    for (ezUInt32 i = 0; i < s_AllCpuScopes.GetCount(); ++i)
    {
      const auto& sourceEventBuffer = s_AllCpuScopes[i];
      CPUScopesBufferFlat& targetEventBuffer = ref_profilingData.m_AllEventBuffers.ExpandAndGetRef();

      targetEventBuffer.m_uiThreadId = sourceEventBuffer->m_uiThreadId;

      ezUInt32 uiSourceCount = sourceEventBuffer->IsMainThread() ? CastToMainThreadEventBuffer(sourceEventBuffer)->m_Data.GetCount() : CastToOtherThreadEventBuffer(sourceEventBuffer)->m_Data.GetCount();
      targetEventBuffer.m_Data.SetCountUninitialized(uiSourceCount);
      for (ezUInt32 j = 0; j < uiSourceCount; ++j)
      {
        const CPUScope& sourceEvent = sourceEventBuffer->IsMainThread() ? CastToMainThreadEventBuffer(sourceEventBuffer)->m_Data[j] : CastToOtherThreadEventBuffer(sourceEventBuffer)->m_Data[j];

        CPUScope& copiedEvent = targetEventBuffer.m_Data[j];
        copiedEvent.m_szFunctionName = sourceEvent.m_szFunctionName;
        copiedEvent.m_BeginTime = sourceEvent.m_BeginTime;
        copiedEvent.m_EndTime = sourceEvent.m_EndTime;
        ezStringUtils::Copy(copiedEvent.m_szName, CPUScope::NAME_SIZE, sourceEvent.m_szName);
      }
    }
  }

  ref_profilingData.m_uiFrameCount = s_uiFrameCount;

  ref_profilingData.m_FrameStartTimes.SetCountUninitialized(s_FrameStartTimes.GetCount());
  for (ezUInt32 i = 0; i < s_FrameStartTimes.GetCount(); ++i)
  {
    ref_profilingData.m_FrameStartTimes[i] = s_FrameStartTimes[i];
  }

  if (!s_GPUScopes.IsEmpty())
  {
    for (const auto& gpuScopes : s_GPUScopes)
    {
      if (gpuScopes != nullptr)
      {
        ezDynamicArray<GPUScope>& gpuScopesCopy = ref_profilingData.m_GPUScopes.ExpandAndGetRef();
        gpuScopesCopy.SetCountUninitialized(gpuScopes->GetCount());
        for (ezUInt32 i = 0; i < gpuScopes->GetCount(); ++i)
        {
          const GPUScope& sourceGpuDat = (*gpuScopes)[i];

          GPUScope& copiedGpuData = gpuScopesCopy[i];
          copiedGpuData.m_BeginTime = sourceGpuDat.m_BeginTime;
          copiedGpuData.m_EndTime = sourceGpuDat.m_EndTime;
          ezStringUtils::Copy(copiedGpuData.m_szName, GPUScope::NAME_SIZE, sourceGpuDat.m_szName);
        }
      }
    }
  }

  if (bClearAfterCapture)
  {
    Clear();
  }
}

// static
void ezProfilingSystem::SetDiscardThreshold(ezTime threshold)
{
  cvar_ProfilingDiscardThresholdMS = static_cast<float>(threshold.GetMilliseconds());
}

void ezProfilingSystem::SetScopeTimeoutCallback(ScopeTimeoutDelegate callback)
{
  s_ScopeTimeoutCallback = callback;
}

// static
ezUInt64 ezProfilingSystem::GetFrameCount()
{
  return s_uiFrameCount;
}

// static
void ezProfilingSystem::StartNewFrame()
{
  ++s_uiFrameCount;

  if (!s_FrameStartTimes.CanAppend())
  {
    s_FrameStartTimes.PopFront();
  }

  s_FrameStartTimes.PushBack(ezTime::Now());

  EZ_PROFILER_FRAME_MARKER();
}

// static
void ezProfilingSystem::AddCPUScope(ezStringView sName, const char* szFunctionName, ezTime beginTime, ezTime endTime, ezTime scopeTimeout)
{
  const ezTime duration = endTime - beginTime;

  // discard?
  if (duration < ezTime::MakeFromMilliseconds(cvar_ProfilingDiscardThresholdMS))
    return;

  ::CpuScopesBufferBase* pScopes = s_CpuScopes;

  if (pScopes == nullptr)
  {
    if (ezThreadUtils::IsMainThread())
    {
      pScopes = EZ_DEFAULT_NEW(::CpuScopesBuffer<BUFFER_SIZE_MAIN_THREAD>);
    }
    else
    {
      pScopes = EZ_DEFAULT_NEW(::CpuScopesBuffer<BUFFER_SIZE_OTHER_THREAD>);
    }

    pScopes->m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
    s_CpuScopes = pScopes;

    {
      EZ_LOCK(s_AllCpuScopesMutex);
      s_AllCpuScopes.PushBack(pScopes);
    }
  }

  CPUScope scope;
  scope.m_szFunctionName = szFunctionName;
  scope.m_BeginTime = beginTime;
  scope.m_EndTime = endTime;
  ezStringUtils::Copy(scope.m_szName, EZ_ARRAY_SIZE(scope.m_szName), sName.GetStartPointer(), sName.GetEndPointer());

  if (ezThreadUtils::IsMainThread())
  {
    auto pMainThreadBuffer = CastToMainThreadEventBuffer(pScopes);
    if (!pMainThreadBuffer->m_Data.CanAppend())
    {
      pMainThreadBuffer->m_Data.PopFront();
    }

    pMainThreadBuffer->m_Data.PushBack(scope);
  }
  else
  {
    auto pOtherThreadBuffer = CastToOtherThreadEventBuffer(pScopes);
    if (!pOtherThreadBuffer->m_Data.CanAppend())
    {
      pOtherThreadBuffer->m_Data.PopFront();
    }

    pOtherThreadBuffer->m_Data.PushBack(scope);
  }

  if (scopeTimeout.IsPositive() && duration > scopeTimeout && s_ScopeTimeoutCallback.IsValid())
  {
    s_ScopeTimeoutCallback(sName, szFunctionName, duration);
  }
}

// static
void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");

  s_MainThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
}

// static
void ezProfilingSystem::Reset()
{
  EZ_LOCK(s_ThreadInfosMutex);
  EZ_LOCK(s_AllCpuScopesMutex);
  for (ezUInt32 i = 0; i < s_DeadThreadIDs.GetCount(); i++)
  {
    ezUInt64 uiThreadId = s_DeadThreadIDs[i];
    for (ezUInt32 k = 0; k < s_ThreadInfos.GetCount(); k++)
    {
      if (s_ThreadInfos[k].m_uiThreadId == uiThreadId)
      {
        // Don't use swap as a thread ID could be re-used and so we might delete the
        // info for an actual thread in the next loop instead of the remnants of the thread
        // that existed before.
        s_ThreadInfos.RemoveAtAndCopy(k);
        break;
      }
    }
    for (ezUInt32 k = 0; k < s_AllCpuScopes.GetCount(); k++)
    {
      CpuScopesBufferBase* pEventBuffer = s_AllCpuScopes[k];
      if (pEventBuffer->m_uiThreadId == uiThreadId)
      {
        EZ_DEFAULT_DELETE(pEventBuffer);
        // Forward order and no swap important, see comment above.
        s_AllCpuScopes.RemoveAtAndCopy(k);
      }
    }
  }
  s_DeadThreadIDs.Clear();
}

// static
void ezProfilingSystem::SetThreadName(ezStringView sThreadName)
{
  EZ_LOCK(s_ThreadInfosMutex);

  ThreadInfo& info = s_ThreadInfos.ExpandAndGetRef();
  info.m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
  info.m_sName = sThreadName;
}

// static
void ezProfilingSystem::RemoveThread()
{
  EZ_LOCK(s_ThreadInfosMutex);

  s_DeadThreadIDs.PushBack((ezUInt64)ezThreadUtils::GetCurrentThreadID());
}

// static
void ezProfilingSystem::InitializeGPUData(ezUInt32 uiGpuCount)
{
  if (s_GPUScopes.GetCount() < uiGpuCount)
  {
    s_GPUScopes.SetCount(uiGpuCount);
  }

  for (auto& gpuScopes : s_GPUScopes)
  {
    if (gpuScopes == nullptr)
    {
      gpuScopes = EZ_DEFAULT_NEW(GPUScopesBuffer);
    }
  }
}

void ezProfilingSystem::AddGPUScope(ezStringView sName, ezTime beginTime, ezTime endTime, ezUInt32 uiGpuIndex)
{
  // discard?
  if (endTime - beginTime < ezTime::MakeFromMilliseconds(cvar_ProfilingDiscardThresholdMS))
    return;

  if (!s_GPUScopes[uiGpuIndex]->CanAppend())
  {
    s_GPUScopes[uiGpuIndex]->PopFront();
  }

  GPUScope scope;
  scope.m_BeginTime = beginTime;
  scope.m_EndTime = endTime;
  ezStringUtils::Copy(scope.m_szName, EZ_ARRAY_SIZE(scope.m_szName), sName.GetStartPointer(), sName.GetEndPointer());

  s_GPUScopes[uiGpuIndex]->PushBack(scope);
}

//////////////////////////////////////////////////////////////////////////

ezProfilingScope::ezProfilingScope(ezStringView sName, const char* szFunctionName, ezTime timeout)
  : m_sName(sName)
  , m_szFunction(szFunctionName)
  , m_BeginTime(ezTime::Now())
  , m_Timeout(timeout)
{
}

ezProfilingScope::~ezProfilingScope()
{
  ezProfilingSystem::AddCPUScope(m_sName, m_szFunction, m_BeginTime, ezTime::Now(), m_Timeout);
}

//////////////////////////////////////////////////////////////////////////

thread_local ezProfilingListScope* ezProfilingListScope::s_pCurrentList = nullptr;

ezProfilingListScope::ezProfilingListScope(ezStringView sListName, ezStringView sFirstSectionName, const char* szFunctionName)
  : m_sListName(sListName)
  , m_szListFunction(szFunctionName)
  , m_ListBeginTime(ezTime::Now())
  , m_sCurSectionName(sFirstSectionName)
  , m_CurSectionBeginTime(m_ListBeginTime)
{
  m_pPreviousList = s_pCurrentList;
  s_pCurrentList = this;
}

ezProfilingListScope::~ezProfilingListScope()
{
  ezTime now = ezTime::Now();
  ezProfilingSystem::AddCPUScope(m_sCurSectionName, nullptr, m_CurSectionBeginTime, now, ezTime::MakeZero());
  ezProfilingSystem::AddCPUScope(m_sListName, m_szListFunction, m_ListBeginTime, now, ezTime::MakeZero());

  s_pCurrentList = m_pPreviousList;
}

// static
void ezProfilingListScope::StartNextSection(ezStringView sNextSectionName)
{
  ezProfilingListScope* pCurScope = s_pCurrentList;

  ezTime now = ezTime::Now();
  ezProfilingSystem::AddCPUScope(pCurScope->m_sCurSectionName, nullptr, pCurScope->m_CurSectionBeginTime, now, ezTime::MakeZero());

  pCurScope->m_sCurSectionName = sNextSectionName;
  pCurScope->m_CurSectionBeginTime = now;
}

#else

ezResult ezProfilingSystem::ProfilingData::Write(ezStreamWriter& outputStream) const
{
  EZ_IGNORE_UNUSED(outputStream);

  return EZ_FAILURE;
}

void ezProfilingSystem::Clear() {}

void ezProfilingSystem::Capture(ezProfilingSystem::ProfilingData& out_Capture, bool bClearAfterCapture)
{
  EZ_IGNORE_UNUSED(out_Capture);
  EZ_IGNORE_UNUSED(bClearAfterCapture);
}

void ezProfilingSystem::SetDiscardThreshold(ezTime threshold)
{
  EZ_IGNORE_UNUSED(threshold);
}

void ezProfilingSystem::StartNewFrame() {}

void ezProfilingSystem::AddCPUScope(ezStringView sName, const char* szFunctionName, ezTime beginTime, ezTime endTime, ezTime scopeTimeout)
{
  EZ_IGNORE_UNUSED(sName);
  EZ_IGNORE_UNUSED(szFunctionName);
  EZ_IGNORE_UNUSED(beginTime);
  EZ_IGNORE_UNUSED(endTime);
  EZ_IGNORE_UNUSED(scopeTimeout);
}

void ezProfilingSystem::Initialize() {}

void ezProfilingSystem::Reset() {}

void ezProfilingSystem::SetThreadName(ezStringView sThreadName)
{
  EZ_IGNORE_UNUSED(sThreadName);
}

void ezProfilingSystem::RemoveThread() {}

void ezProfilingSystem::InitializeGPUData(ezUInt32 gpuCount)
{
  EZ_IGNORE_UNUSED(gpuCount);
}

void ezProfilingSystem::AddGPUScope(ezStringView sName, ezTime beginTime, ezTime endTime, ezUInt32 gpuIndex)
{
  EZ_IGNORE_UNUSED(sName);
  EZ_IGNORE_UNUSED(beginTime);
  EZ_IGNORE_UNUSED(endTime);
  EZ_IGNORE_UNUSED(gpuIndex);
}

void ezProfilingSystem::ProfilingData::Merge(ProfilingData& out_Merged, ezArrayPtr<const ProfilingData*> inputs)
{
  EZ_IGNORE_UNUSED(out_Merged);
  EZ_IGNORE_UNUSED(inputs);
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);
