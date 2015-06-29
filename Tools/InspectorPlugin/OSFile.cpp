#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/IO/OSFile.h>

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
    }
    break;

  case ezOSFile::EventType::FileExists:
  case ezOSFile::EventType::DirectoryExists:
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

  case ezOSFile::EventType::FileStat:
    {
      Msg.SetMessageID('FILE', 'STAT');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

  case ezOSFile::EventType::FileCasing:
    {
      Msg.SetMessageID('FILE', 'CASE');
      Msg.GetWriter() << e.m_szFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;
  }

  ezUInt8 uiThreadType = 0;

  if (ezThreadUtils::IsMainThread())
    uiThreadType = 1 << 0;
  else
  if (ezTaskSystem::IsLoadingThread())
    uiThreadType = 1 << 1;
  else
    uiThreadType = 1 << 2;

  Msg.GetWriter() << e.m_Duration.GetSeconds();
  Msg.GetWriter() << uiThreadType;

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



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_OSFile);

