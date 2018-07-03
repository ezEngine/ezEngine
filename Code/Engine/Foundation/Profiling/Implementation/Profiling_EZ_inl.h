
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

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
    ezUInt64 m_uiThreadId;
    ezTime m_TimeStamp;
    ezUInt32 m_uiNextIndex : 31;
    ezUInt32 m_Type : 1;
    char m_szName[36];
  };

  // EZ_CHECK_AT_COMPILETIME(sizeof(Event) == 64);

  struct EventBuffer
  {
    ezStaticRingBuffer<Event, 1024 * 512 / sizeof(Event)> m_Data;
    ezUInt64 m_uiThreadId = 0;
  };

  static thread_local EventBuffer* s_EventBuffers = nullptr;
  static ezDynamicArray<EventBuffer*> s_AllEventBuffers;
  static ezMutex s_AllEventBuffersMutex;

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
    e.m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
    e.m_TimeStamp = ezTime::Now();
    e.m_uiNextIndex = 0xFFFFFF;
    e.m_Type = type;
    ezStringUtils::CopyN(e.m_szName, EZ_ARRAY_SIZE(e.m_szName), szName, uiNameLength);

    pEventBuffer->m_Data.PushBack(e);

    return pEventBuffer->m_Data.PeekBack();
  }
}

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
        s_ThreadInfos.RemoveAt(k);
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
        s_AllEventBuffers.RemoveAt(k);
      }
    }
  }
  s_DeadThreadIDs.Clear();
}

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
void ezProfilingSystem::Capture(ezStreamWriter& outputStream)
{
  ezStandardJSONWriter writer;
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
        for (ezUInt32 i = 0; i < pEventBuffer->m_Data.GetCount(); ++i)
        {
          const Event& e = pEventBuffer->m_Data[i];

          writer.BeginObject();
          writer.AddVariableString("name", e.m_szName);
          writer.AddVariableUInt32("pid", 1); // dummy value
          writer.AddVariableUInt64("tid", e.m_uiThreadId);
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

    writer.EndArray();
  }

  writer.EndObject();
}
