#include <PCH.h>
#include <Foundation/Threading/ThreadUtils.h>

static void OSFileEventHandler(const ezOSFile::EventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage Msg;
  Msg.GetWriter() << e.m_iFileID;

  switch (e.m_EventType)
  {
  case ezOSFile::EventType::FileOpen:
    {
      Msg.SetMessageID('FILE', 'OPEN');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << (ezUInt8) e.m_FileMode;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

  case ezOSFile::EventType::FileRead:
    {
      Msg.SetMessageID('FILE', 'READ');
      Msg.GetWriter() << e.m_uiBytesAccessed;
      Msg.GetWriter() << (e.m_bSuccess ? e.m_uiBytesAccessed : (ezUInt64) 0);
    }
    break;

  case ezOSFile::EventType::FileWrite:
    {
      Msg.SetMessageID('FILE', 'WRIT');
      Msg.GetWriter() << e.m_uiBytesAccessed;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

  case ezOSFile::EventType::FileClose:
    {
      Msg.SetMessageID('FILE', 'CLOS');
      Msg.GetWriter() << e.m_iFileID;
    }
    break;

  case ezOSFile::EventType::FileExists:
    {
      Msg.SetMessageID('FILE', 'EXST');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << e.m_bSuccess;

    }
    break;

  case ezOSFile::EventType::FileDelete:
    {
      Msg.SetMessageID('FILE', 'DEL');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

  case ezOSFile::EventType::MakeDir:
    {
      Msg.SetMessageID('FILE', 'CDIR');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

  case ezOSFile::EventType::FileCopy:
    {
      Msg.SetMessageID('FILE', 'COPY');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << e.m_szFile2;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;
  }

  Msg.GetWriter() << e.m_Duration.GetSeconds();
  Msg.GetWriter() << ezThreadUtils::IsMainThread();

  ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
}

void AddOSFileEventHandler()
{
  ezOSFile::AddEventHandler(OSFileEventHandler);
}

void RemoveOSFileEventHandler()
{
  ezOSFile::RemoveEventHandler(OSFileEventHandler);
}

