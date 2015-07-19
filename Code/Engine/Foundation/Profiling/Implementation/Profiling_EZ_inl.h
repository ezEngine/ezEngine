
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

namespace
{
  struct ProfilingInfo
  {
    ProfilingInfo(const char* szName) : m_sName(szName)
    {
    }

    ezString m_sName;
  };

  struct ThreadInfo
  {
    ezUInt64 m_uiThreadId;
    ezString m_sName;
  };

  ezHybridArray<ThreadInfo, 16> g_Threads;

  struct CapturedEvent
  {
    EZ_DECLARE_POD_TYPE();

    enum Type
    {
      Begin,
      End
    };

    const char* m_szName;
    const char* m_szFilename;
    const char* m_szFunctionName;
    ezUInt32 m_uiLineNumber : 31;
    ezUInt32 m_Type : 1;
    ezUInt64 m_uiThreadId;
    ezTime m_TimeStamp;
  };

  // reserve 1MB for captured events
  ezStaticRingBuffer<CapturedEvent, (1024*1024) / sizeof(CapturedEvent)> g_CapturedEvents;

  ezMutex g_CaptureMutex;
}


ezProfilingScope::ezProfilingScope(const ezProfilingId& id, const char* szFileName,
  const char* szFunctionName, ezUInt32 uiLineNumber) :
  m_Id(id)
{
  EZ_LOCK(g_CaptureMutex);
  m_szName = GetProfilingInfo(id.m_Id).m_sName.GetData();

  if (!g_CapturedEvents.CanAppend())
    g_CapturedEvents.PopFront();

  CapturedEvent e;
  e.m_szName = m_szName;
  e.m_szFilename = szFileName;
  e.m_szFunctionName = szFunctionName;
  e.m_uiLineNumber = uiLineNumber;
  e.m_Type = CapturedEvent::Begin;
  e.m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
  e.m_TimeStamp = ezTime::Now();

  g_CapturedEvents.PushBack(e);
}

ezProfilingScope::~ezProfilingScope()
{
  EZ_LOCK(g_CaptureMutex);

  if (!g_CapturedEvents.CanAppend())
    g_CapturedEvents.PopFront();

  CapturedEvent e;
  e.m_szName = m_szName;
  e.m_szFilename = nullptr;
  e.m_szFunctionName = nullptr;
  e.m_uiLineNumber = 0;
  e.m_Type = CapturedEvent::End;
  e.m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
  e.m_TimeStamp = ezTime::Now();

  g_CapturedEvents.PushBack(e);
}

//static 
void ezProfilingSystem::SetThreadName(const char* szThreadName)
{
  EZ_LOCK(g_CaptureMutex);

  ThreadInfo info;
  info.m_uiThreadId = (ezUInt64)ezThreadUtils::GetCurrentThreadID();
  info.m_sName = szThreadName;

  g_Threads.PushBack(info);
}

//static
void ezProfilingSystem::Capture(ezStreamWriterBase& outputStream)
{
  EZ_LOCK(g_CaptureMutex);

  ezStandardJSONWriter writer;
  writer.SetOutputStream(&outputStream);

  writer.BeginObject();
  {
    writer.BeginArray("traceEvents");
    for (ezUInt32 i = 0; i < g_Threads.GetCount(); ++i)
    {
      const ThreadInfo& info = g_Threads[i];

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

    for (ezUInt32 i = 0; i < g_CapturedEvents.GetCount(); ++i)
    {
      const CapturedEvent& e = g_CapturedEvents[i];

      writer.BeginObject();
      writer.AddVariableString("name", e.m_szName);
      writer.AddVariableUInt32("pid", 1); // dummy value
      writer.AddVariableUInt64("tid", e.m_uiThreadId);
      writer.AddVariableUInt64("ts", static_cast<ezUInt64>(e.m_TimeStamp.GetMicroseconds()));
      writer.AddVariableString("ph", e.m_Type == CapturedEvent::Begin ? "B" : "E");     
      
      if (e.m_szFilename != nullptr)
      {
        writer.BeginObject("args");
        writer.AddVariableString("file", e.m_szFilename);
        writer.AddVariableString("function", e.m_szFunctionName);
        writer.AddVariableUInt32("line", e.m_uiLineNumber);
        writer.EndObject();
      }

      writer.EndObject();
    }
    writer.EndArray();
  }

  writer.EndObject();
}

