#include <FoundationPCH.h>

#include <Foundation/Communication/DataTransfer.h>
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


    const ezProfilingSystem::ProfilingData profilingData = ezProfilingSystem::Capture();
    profilingData.Write(dto.GetWriter());

    dto.Transmit();
  }
};

static ezProfileCaptureDataTransfer s_ProfileCaptureDataTransfer;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    ezProfilingSystem::Initialize();
    s_ProfileCaptureDataTransfer.EnableDataTransfer("Profiling Capture");
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_ProfileCaptureDataTransfer.DisableDataTransfer();
    ezProfilingSystem::Reset();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  enum
  {
    RING_BUFFER_SIZE_PER_THREAD = 1024 * 1024
  };

  enum
  {
    RING_BUFFER_SIZE_FRAMES = 120 * 60,
  };

  typedef ezStaticRingBuffer<ezProfilingSystem::GPUData, RING_BUFFER_SIZE_PER_THREAD / sizeof(ezProfilingSystem::GPUData)> GPUDataRingBuffer;

  struct EventBuffer
  {
    ezStaticRingBuffer<ezProfilingSystem::Event, RING_BUFFER_SIZE_PER_THREAD / sizeof(ezProfilingSystem::Event)> m_Data;
    ezUInt64 m_uiThreadId = 0;
  };

  ezStaticRingBuffer<ezTime, RING_BUFFER_SIZE_FRAMES> s_FrameStartTimes;
  ezUInt64 s_uiFrameCount = 0;

  static ezHybridArray<ezProfilingSystem::ThreadInfo, 16> s_ThreadInfos;
  static ezHybridArray<ezUInt64, 16> s_DeadThreadIDs;
  static ezMutex s_ThreadInfosMutex;

#  if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(ezProfilingSystem::Event) == 64);
  EZ_CHECK_AT_COMPILETIME(sizeof(ezProfilingSystem::GPUData) == 64);
#  endif
   
  static thread_local ::EventBuffer* s_EventBuffers = nullptr;
  static ezDynamicArray<::EventBuffer*> s_AllEventBuffers;
  static ezMutex s_AllEventBuffersMutex;

  static GPUDataRingBuffer* s_GPUData;

  ezProfilingSystem::Event& AllocateEvent(const char* szName, ezUInt32 uiNameLength, ezProfilingSystem::Event::Type type)
  {
    ::EventBuffer* pEventBuffer = s_EventBuffers;

    if (pEventBuffer == nullptr)
    {
      pEventBuffer = EZ_DEFAULT_NEW(::EventBuffer);
      pEventBuffer->m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
      s_EventBuffers = pEventBuffer;

      {
        EZ_LOCK(s_AllEventBuffersMutex);
        s_AllEventBuffers.PushBack(pEventBuffer);
      }
    }

    if (!pEventBuffer->m_Data.CanAppend())
    {
      pEventBuffer->m_Data.PopFront();
    }

    ezProfilingSystem::Event e;
    e.m_szFunctionName = nullptr;
    e.m_TimeStamp = ezTime::Now();
    e.m_Type = type;
    ezStringUtils::CopyN(e.m_szName, EZ_ARRAY_SIZE(e.m_szName), szName, uiNameLength);

    pEventBuffer->m_Data.PushBack(e);

    return pEventBuffer->m_Data.PeekBack();
  }
} // namespace

ezResult ezProfilingSystem::ProfilingData::Write(ezStreamWriter& outputStream) const
{
  ezStandardJSONWriter writer;
  writer.SetWhitespaceMode(ezJSONWriter::WhitespaceMode::None);
  writer.SetOutputStream(&outputStream);

  writer.BeginObject();
  {
    writer.BeginArray("traceEvents");

    {
      // Frames thread
      {
        writer.BeginObject();
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", "Frames");
        writer.EndObject();

        writer.EndObject();

        writer.BeginObject();
        writer.AddVariableString("name", "thread_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", -1);
        writer.EndObject();

        writer.EndObject();

        if (writer.HadWriteError())
        {
          return EZ_FAILURE;
        }
      }

      // GPU thread
      {
        writer.BeginObject();
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiGPUThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", "GPU");
        writer.EndObject();

        writer.EndObject();

        writer.BeginObject();
        writer.AddVariableString("name", "thread_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiGPUThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", -2);
        writer.EndObject();

        writer.EndObject();
        if (writer.HadWriteError())
        {
          return EZ_FAILURE;
        }
      }

      {
        for (const ThreadInfo& info : m_ThreadInfos)
        {
          writer.BeginObject();
          writer.AddVariableString("name", "thread_name");
          writer.AddVariableString("cat", "__metadata");
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", info.m_uiThreadId + 2);
          writer.AddVariableString("ph", "M");

          writer.BeginObject("args");
          writer.AddVariableString("name", info.m_sName);
          writer.EndObject();

          writer.EndObject();

          if (writer.HadWriteError())
          {
            return EZ_FAILURE;
          }
        }
      }
    }

    {
      for (const auto& eventBuffer : m_AllEventBuffers)
      {
        const ezUInt64 uiThreadId = eventBuffer.m_uiThreadId + 2;
        for (ezUInt32 i = 0; i < eventBuffer.m_Data.GetCount(); ++i)
        {
          const Event& e = eventBuffer.m_Data[i];

          writer.BeginObject();
          writer.AddVariableString("name", e.m_szName);
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", uiThreadId);
          writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_TimeStamp.GetMicroseconds()));
          writer.AddVariableString("ph", e.m_Type == Event::Begin ? "B" : "E");

          if (e.m_szFunctionName != nullptr)
          {
            writer.BeginObject("args");
            writer.AddVariableString("function", e.m_szFunctionName);
            writer.EndObject();
          }

          writer.EndObject();
          if (writer.HadWriteError())
          {
            return EZ_FAILURE;
          }
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
        sFrameName.Format("Frame {}", m_uiFrameCount - localFrameID);

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
    {
      for (ezUInt32 i = 0; i < m_GPUData.GetCount(); ++i)
      {
        const auto& e = m_GPUData[i];

        writer.BeginObject();
        writer.AddVariableString("name", e.m_szName);
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiGPUThreadID);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_BeginTime.GetMicroseconds()));
        writer.AddVariableString("ph", "B");
        writer.EndObject();

        writer.BeginObject();
        writer.AddVariableString("name", e.m_szName);
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiGPUThreadID);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_EndTime.GetMicroseconds()));
        writer.AddVariableString("ph", "E");
        writer.EndObject();
        if (writer.HadWriteError())
        {
          return EZ_FAILURE;
        }
      }
    }

    writer.EndArray();
  }

  writer.EndObject();

  return writer.HadWriteError() ? EZ_FAILURE : EZ_SUCCESS;
}

// static
ezProfilingSystem::ProfilingData ezProfilingSystem::Capture()
{
  ezProfilingSystem::ProfilingData profilingData;

  profilingData.m_uiFramesThreadID = 1;
  profilingData.m_uiGPUThreadID = 0;
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  profilingData.m_uiProcessID = ezProcess::GetCurrentProcessID();
#else
  profilingData.m_uiProcessID = 0;
#endif

  {
    EZ_LOCK(s_ThreadInfosMutex);
    profilingData.m_ThreadInfos = s_ThreadInfos;
  }

  {
    EZ_LOCK(s_AllEventBuffersMutex);

    profilingData.m_AllEventBuffers.Reserve(s_AllEventBuffers.GetCount());
    for (ezUInt32 i = 0; i < s_AllEventBuffers.GetCount(); ++i)
    {
      auto& sourceEventBuffer = s_AllEventBuffers[i];
      EventBufferFlat targetEventBuffer;

      targetEventBuffer.m_uiThreadId = sourceEventBuffer->m_uiThreadId;

      targetEventBuffer.m_Data.Reserve(sourceEventBuffer->m_Data.GetCount());
      for (ezUInt32 j = 0; j < sourceEventBuffer->m_Data.GetCount(); ++j)
      {
        const Event& sourceEvent = sourceEventBuffer->m_Data[j];

        Event copiedEvent;
        copiedEvent.m_szFunctionName = sourceEvent.m_szFunctionName;
        copiedEvent.m_TimeStamp = sourceEvent.m_TimeStamp;
        ezStringUtils::Copy(copiedEvent.m_szName, Event::NAME_SIZE, sourceEvent.m_szName);
        copiedEvent.m_Type = sourceEvent.m_Type;

        targetEventBuffer.m_Data.PushBack(std::move(copiedEvent));
      }

      profilingData.m_AllEventBuffers.PushBack(std::move(targetEventBuffer));
    }
  }

  profilingData.m_uiFrameCount = s_uiFrameCount;

  profilingData.m_FrameStartTimes.Reserve(profilingData.m_FrameStartTimes.GetCount());
  for (ezUInt32 i = 0; i < s_FrameStartTimes.GetCount(); ++i)
  {
    profilingData.m_FrameStartTimes.PushBack(s_FrameStartTimes[i]);
  }

  profilingData.m_GPUData.Reserve(s_GPUData->GetCount());
  for (ezUInt32 i = 0; i < s_GPUData->GetCount(); ++i)
  {
    const GPUData& sourceGpuDat = (*s_GPUData)[i];

    GPUData copiedGpuData;
    copiedGpuData.m_BeginTime = sourceGpuDat.m_BeginTime;
    copiedGpuData.m_EndTime = sourceGpuDat.m_EndTime;
    ezStringUtils::Copy(copiedGpuData.m_szName, GPUData::NAME_SIZE, sourceGpuDat.m_szName);

    profilingData.m_GPUData.PushBack(std::move(copiedGpuData));
  }

  return profilingData;
}

// static
void ezProfilingSystem::Reset()
{
  EZ_LOCK(s_ThreadInfosMutex);
  EZ_LOCK(s_AllEventBuffersMutex);
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
    for (ezUInt32 k = 0; k < s_AllEventBuffers.GetCount(); k++)
    {
      EventBuffer* pEventBuffer = s_AllEventBuffers[k];
      if (pEventBuffer->m_uiThreadId == uiThreadId)
      {
        EZ_DEFAULT_DELETE(pEventBuffer);
        // Forward order and no swap important, see comment above.
        s_AllEventBuffers.RemoveAtAndCopy(k);
      }
    }
  }
  s_DeadThreadIDs.Clear();
}

// static
void ezProfilingSystem::SetThreadName(const char* szThreadName)
{
  EZ_LOCK(s_ThreadInfosMutex);

  ThreadInfo& info = s_ThreadInfos.ExpandAndGetRef();
  info.m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
  info.m_sName = szThreadName;
}

// static
void ezProfilingSystem::RemoveThread()
{
  EZ_LOCK(s_ThreadInfosMutex);

  s_DeadThreadIDs.PushBack((ezUInt64)ezThreadUtils::GetCurrentThreadID());
}

// static
void ezProfilingSystem::InitializeGPUData()
{
  if (s_GPUData == nullptr)
  {
    s_GPUData = EZ_DEFAULT_NEW(GPUDataRingBuffer);
  }
}

ezProfilingSystem::GPUData& ezProfilingSystem::AllocateGPUData()
{
  if (!s_GPUData->CanAppend())
  {
    s_GPUData->PopFront();
  }

  s_GPUData->PushBack(ezProfilingSystem::GPUData());
  return s_GPUData->PeekBack();
}

//////////////////////////////////////////////////////////////////////////

ezProfilingScope::ezProfilingScope(const char* szName, const char* szFunctionName)
  : m_szName(szName)
  , m_uiNameLength((ezUInt32)::strlen(szName))
{
  ezProfilingSystem::BeginScope(m_szName, m_uiNameLength, szFunctionName);
}

ezProfilingScope::~ezProfilingScope()
{
  ezProfilingSystem::EndScope(m_szName, m_uiNameLength);
}

//////////////////////////////////////////////////////////////////////////

thread_local ezProfilingListScope* ezProfilingListScope::s_pCurrentList = nullptr;

ezProfilingListScope::ezProfilingListScope(const char* szListName, const char* szFirstSectionName, const char* szFunctionName)
  : m_szListName(szListName)
  , m_uiListNameLength((ezUInt32)::strlen(szListName))
  , m_szCurSectionName(szFirstSectionName)
  , m_uiCurSectionNameLength((ezUInt32)::strlen(szFirstSectionName))
{
  m_pPreviousList = s_pCurrentList;
  s_pCurrentList = this;

  {
    ezProfilingSystem::Event& e = AllocateEvent(m_szListName, m_uiListNameLength, ezProfilingSystem::Event::Begin);
    e.m_szFunctionName = szFunctionName;
  }

  {
    ezProfilingSystem::Event& e = AllocateEvent(m_szCurSectionName, m_uiCurSectionNameLength, ezProfilingSystem::Event::Begin);
    e.m_szFunctionName = nullptr;
  }
}

ezProfilingListScope::~ezProfilingListScope()
{
  {
    ezProfilingSystem::Event& e = AllocateEvent(m_szCurSectionName, m_uiCurSectionNameLength, ezProfilingSystem::Event::End);
    e.m_szFunctionName = nullptr;
  }

  {
    ezProfilingSystem::Event& e = AllocateEvent(m_szListName, m_uiListNameLength, ezProfilingSystem::Event::End);
    e.m_szFunctionName = nullptr;
  }

  s_pCurrentList = m_pPreviousList;
}

void ezProfilingListScope::StartNextSection(const char* szNextSectionName)
{
  ezProfilingListScope* pCurScope = s_pCurrentList;

  {
    ezProfilingSystem::Event& e = AllocateEvent(pCurScope->m_szCurSectionName, pCurScope->m_uiCurSectionNameLength, ezProfilingSystem::Event::End);
    e.m_szFunctionName = nullptr;
  }

  pCurScope->m_szCurSectionName = szNextSectionName;
  pCurScope->m_uiCurSectionNameLength = (ezUInt32)::strlen(szNextSectionName);

  {
    ezProfilingSystem::Event& e = AllocateEvent(pCurScope->m_szCurSectionName, pCurScope->m_uiCurSectionNameLength, ezProfilingSystem::Event::Begin);
    e.m_szFunctionName = nullptr;
  }
}



// static
void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");
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
}

void ezProfilingSystem::BeginScope(const char* szName, ezUInt32 uiNameLength, const char* szFunctionName)
{
  Event& e = AllocateEvent(szName, uiNameLength, Event::Begin);
  e.m_szFunctionName = szFunctionName;
}

void ezProfilingSystem::EndScope(const char* szName, ezUInt32 uiNameLegth)
{
  Event& e = AllocateEvent(szName, uiNameLegth, Event::End);
  e.m_szFunctionName = nullptr;
}

#else

// static
void ezProfilingSystem::SetThreadName(const char* szThreadName) {}

void ezProfilingSystem::Initialize() {}

void ezProfilingSystem::Reset() {}

ezResult ezProfilingSystem::ProfilingData::Write(ezStreamWriter& outputStream) const { return EZ_FAILURE; }

ezProfilingSystem::ProfilingData ezProfilingSystem::Capture() { return {}; }

void ezProfilingSystem::RemoveThread() {}

void ezProfilingSystem::StartNewFrame() {}

void ezProfilingSystem::BeginScope(const char* szName, ezUInt32 uiNameLength, const char* szFunctionName) {}

void ezProfilingSystem::EndScope(const char* szName, ezUInt32 uiNameLegth) {}

void ezProfilingSystem::InitializeGPUData() {}

static ezProfilingSystem::GPUData s_Dummy;
ezProfilingSystem::GPUData& ezProfilingSystem::AllocateGPUData()
{
  return s_Dummy;
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);

