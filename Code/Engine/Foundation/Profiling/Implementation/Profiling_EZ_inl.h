
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  struct ThreadInfo
  {
    ezUInt64 m_uiThreadId;
    ezString m_sName;
  };

  static ezHybridArray<ThreadInfo, 16> s_ThreadInfos;
  static ezHybridArray<ezUInt64, 16> s_DeadThreadIDs;
  static ezMutex s_ThreadInfosMutex;

  struct Event
  {
    EZ_DECLARE_POD_TYPE();

    enum Type
    {
      Begin,
      End
    };

    const char* m_szFunctionName;
    ezTime m_TimeStamp;
    char m_szName[47];
    ezUInt8 m_Type;
  };

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(Event) == 64);
  EZ_CHECK_AT_COMPILETIME(sizeof(ezProfilingSystem::GPUData) == 64);
#endif

  enum
  {
    RING_BUFFER_SIZE_PER_THREAD = 1024 * 1024
  };

  struct EventBuffer
  {
    ezStaticRingBuffer<Event, RING_BUFFER_SIZE_PER_THREAD / sizeof(Event)> m_Data;
    ezUInt64 m_uiThreadId = 0;
  };

  static thread_local EventBuffer* s_EventBuffers = nullptr;
  static ezDynamicArray<EventBuffer*> s_AllEventBuffers;
  static ezMutex s_AllEventBuffersMutex;

  typedef ezStaticRingBuffer<ezProfilingSystem::GPUData, RING_BUFFER_SIZE_PER_THREAD / sizeof(ezProfilingSystem::GPUData)> GPUDataRingBuffer;
  static GPUDataRingBuffer* s_GPUData;

  Event& AllocateEvent(const char* szName, ezUInt32 uiNameLength, Event::Type type)
  {
    EventBuffer* pEventBuffer = s_EventBuffers;

    if (pEventBuffer == nullptr)
    {
      pEventBuffer = EZ_DEFAULT_NEW(EventBuffer);
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

    Event e;
    e.m_szFunctionName = nullptr;
    e.m_TimeStamp = ezTime::Now();
    e.m_Type = type;
    ezStringUtils::CopyN(e.m_szName, EZ_ARRAY_SIZE(e.m_szName), szName, uiNameLength);

    pEventBuffer->m_Data.PushBack(e);

    return pEventBuffer->m_Data.PeekBack();
  }
} // namespace

// static
void ezProfilingSystem::Capture(ezStreamWriter& outputStream)
{
  ezStandardJSONWriter writer;
  writer.SetWhitespaceMode(ezJSONWriter::WhitespaceMode::None);
  writer.SetOutputStream(&outputStream);

  writer.BeginObject();
  {
    writer.BeginArray("traceEvents");

    {
      EZ_LOCK(s_ThreadInfosMutex);

      for (const ThreadInfo& info : s_ThreadInfos)
      {
        writer.BeginObject();
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", 1); // dummy value
        writer.AddVariableUInt64("tid", info.m_uiThreadId);
        writer.AddVariableUInt64("ts", 0);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", info.m_sName.GetData());
        writer.EndObject();

        writer.EndObject();
      }
    }

    {
      EZ_LOCK(s_AllEventBuffersMutex);

      for (auto pEventBuffer : s_AllEventBuffers)
      {
        ezUInt64 uiThreadId = pEventBuffer->m_uiThreadId;
        for (ezUInt32 i = 0; i < pEventBuffer->m_Data.GetCount(); ++i)
        {
          const Event& e = pEventBuffer->m_Data[i];

          writer.BeginObject();
          writer.AddVariableString("name", e.m_szName);
          writer.AddVariableUInt32("pid", 1); // dummy value
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
        }
      }
    }

    {
      for (ezUInt32 i = 0; i < s_GPUData->GetCount(); ++i)
      {
        const auto& e = (*s_GPUData)[i];

        writer.BeginObject();
        writer.AddVariableString("name", e.m_szName);
        writer.AddVariableUInt32("pid", 1); // dummy value
        writer.AddVariableUInt64("tid", 0);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_BeginTime.GetMicroseconds()));
        writer.AddVariableString("ph", "B");
        writer.EndObject();

        writer.BeginObject();
        writer.AddVariableString("name", e.m_szName);
        writer.AddVariableUInt32("pid", 1); // dummy value
        writer.AddVariableUInt64("tid", 0);
        writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_EndTime.GetMicroseconds()));
        writer.AddVariableString("ph", "E");
        writer.EndObject();
      }
    }

    writer.EndArray();
  }

  writer.EndObject();
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
  s_GPUData = EZ_DEFAULT_NEW(GPUDataRingBuffer);

  EZ_LOCK(s_ThreadInfosMutex);

  ThreadInfo& info = s_ThreadInfos.ExpandAndGetRef();
  info.m_uiThreadId = 0;
  info.m_sName = "GPU";
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
  Event& e = AllocateEvent(m_szName, m_uiNameLength, Event::Begin);
  e.m_szFunctionName = szFunctionName;
}

ezProfilingScope::~ezProfilingScope()
{
  Event& e = AllocateEvent(m_szName, m_uiNameLength, Event::End);
  e.m_szFunctionName = nullptr;
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
    Event& e = AllocateEvent(m_szListName, m_uiListNameLength, Event::Begin);
    e.m_szFunctionName = szFunctionName;
  }

  {
    Event& e = AllocateEvent(m_szCurSectionName, m_uiCurSectionNameLength, Event::Begin);
    e.m_szFunctionName = nullptr;
  }
}

ezProfilingListScope::~ezProfilingListScope()
{
  {
    Event& e = AllocateEvent(m_szCurSectionName, m_uiCurSectionNameLength, Event::End);
    e.m_szFunctionName = nullptr;
  }

  {
    Event& e = AllocateEvent(m_szListName, m_uiListNameLength, Event::End);
    e.m_szFunctionName = nullptr;
  }

  s_pCurrentList = m_pPreviousList;
}

void ezProfilingListScope::StartNextSection(const char* szNextSectionName)
{
  ezProfilingListScope* pCurScope = s_pCurrentList;

  {
    Event& e = AllocateEvent(pCurScope->m_szCurSectionName, pCurScope->m_uiCurSectionNameLength, Event::End);
    e.m_szFunctionName = nullptr;
  }

  pCurScope->m_szCurSectionName = szNextSectionName;
  pCurScope->m_uiCurSectionNameLength = (ezUInt32)::strlen(szNextSectionName);

  {
    Event& e = AllocateEvent(pCurScope->m_szCurSectionName, pCurScope->m_uiCurSectionNameLength, Event::Begin);
    e.m_szFunctionName = nullptr;
  }
}
