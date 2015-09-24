#include <PCH.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Stats.h>
#include <QCoreApplication>
#include <QProcess>
#include <QSharedMemory>
#include <Foundation/Threading/ThreadUtils.h>

ezProcessCommunication::ezProcessCommunication()
{
  m_pClientProcess = nullptr;
  m_pSharedMemory = nullptr;
  m_uiProcessID = 0;
  m_pWaitForMessageType = nullptr;
}

ezResult ezProcessCommunication::StartClientProcess(const char* szProcess, const char* szArguments, ezUInt32 uiMemSize)
{
  EZ_LOG_BLOCK("ezProcessCommunication::StartClientProcess");

  EZ_ASSERT_DEV(m_pSharedMemory == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_uiProcessID = 0xF0F0F0F0;

  m_pSharedMemory = new QSharedMemory();

  ezStringBuilder sMemName;
  for (ezUInt32 i = 0; i < 1000; ++i)
  {
    sMemName.Format("%4X", i);

    m_pSharedMemory->setKey(QLatin1String(sMemName.GetData()));

    if (m_pSharedMemory->create(uiMemSize))
      goto success;
  }

  ezLog::Error("Could not find shared memory to use");
  return EZ_FAILURE;

success:

  EZ_VERIFY(m_pSharedMemory->lock(), "Implementation error?");
  ezMemoryUtils::ZeroFill<ezUInt8>((ezUInt8*)m_pSharedMemory->data(), m_pSharedMemory->size());
  EZ_VERIFY(m_pSharedMemory->unlock(), "Implementation error?");

  ezStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = ezOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  ezString sPID = ezConversionUtils::ToString((ezUInt64)QCoreApplication::applicationPid());

  QStringList arguments;
  arguments << "-IPC";
  arguments << QLatin1String(sMemName.GetData());
  arguments << "-PID";
  arguments << sPID.GetData();

  if (!ezStringUtils::IsNullOrEmpty(szArguments))
    arguments << QString::fromUtf8(szArguments);

  m_pClientProcess = new QProcess();
  m_pClientProcess->start(QString::fromUtf8(sPath.GetData()), arguments);

  if (!m_pClientProcess->waitForStarted())
  {
    delete m_pClientProcess;
    m_pClientProcess = nullptr;

    delete m_pSharedMemory;
    m_pSharedMemory = nullptr;

    ezLog::Error("Failed to start process '%s'", sPath.GetData());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProcessCommunication::ConnectToHostProcess()
{
  EZ_ASSERT_DEV(m_pSharedMemory == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_uiProcessID = 0x0A0A0A0A;

  if (ezStringUtils::IsNullOrEmpty(ezCommandLineUtils::GetInstance()->GetStringOption("-IPC")))
  {
    EZ_REPORT_FAILURE("Command Line does not contain -IPC parameter");
    return EZ_FAILURE;
  }

  if (ezStringUtils::IsNullOrEmpty(ezCommandLineUtils::GetInstance()->GetStringOption("-PID")))
  {
    EZ_REPORT_FAILURE("Command Line does not contain -PID parameter");
    return EZ_FAILURE;
  }

  m_iHostPID = 0;
  ezConversionUtils::StringToInt64(ezCommandLineUtils::GetInstance()->GetStringOption("-PID"), m_iHostPID);

  ezLog::Debug("Host Process ID: %lli", m_iHostPID);

  m_pSharedMemory = new QSharedMemory(QLatin1String(ezCommandLineUtils::GetInstance()->GetStringOption("-IPC")));
  if (!m_pSharedMemory->attach())
  {
    delete m_pSharedMemory;
    m_pSharedMemory = nullptr;

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezProcessCommunication::CloseConnection()
{
  EZ_LOCK(m_SendQueueMutex);

  m_uiProcessID = 0;

  // If we kill the process while it is in the shared memory lock we will be unable to lock
  // the shared memory ourselves. So we kill the process after freeing us from the shared memory.
  delete m_pSharedMemory;
  m_pSharedMemory = nullptr;

  delete m_pClientProcess;
  m_pClientProcess = nullptr;

  m_MessageSendQueue.Clear();
  m_MessageReadQueue.Clear();
}

bool ezProcessCommunication::IsHostAlive() const
{
  if (m_iHostPID == 0)
    return false;

  bool bValid = true;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_iHostPID);
  bValid = (hProcess != INVALID_HANDLE_VALUE) && (hProcess != NULL);

  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) && exitcode != STILL_ACTIVE)
    bValid = false;

  CloseHandle(hProcess);
#endif

  return bValid;
}

bool ezProcessCommunication::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void ezProcessCommunication::SendMessage(ezProcessMessage* pMessage, bool bSuperHighPriority)
{
  {
    EZ_LOCK(m_SendQueueMutex);

    if (m_pSharedMemory == nullptr)
      return;

    pMessage->m_iSentTimeStamp = ezTimestamp::CurrentTimestamp().GetInt64(ezSIUnitOfTime::Microsecond);

    {
      ezMemoryStreamStorage& storage = m_MessageSendQueue.ExpandAndGetRef();
      ezMemoryStreamWriter writer(&storage);

      ezReflectionSerializer::WriteObjectToJSON(writer, pMessage->GetDynamicRTTI(), pMessage);
    }
  }

  if (bSuperHighPriority)
  {
    EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function is not thread safe");
    ProcessMessages(false);
  }
}

bool ezProcessCommunication::ProcessMessages(bool bAllowMsgDispatch)
{
  if (!m_pSharedMemory)
    return false;

  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function is not thread safe");

  EZ_VERIFY(m_pSharedMemory->lock(), "Implementation error?");

  ezUInt8* pData = (ezUInt8*)m_pSharedMemory->data();

  if (ReadMessages())
    WriteMessages();

  EZ_VERIFY(m_pSharedMemory->unlock(), "Implementation error?");

  if (bAllowMsgDispatch && !m_MessageReadQueue.IsEmpty())
  {
    DispatchMessages();
    return true;
  }

  return false;
}

void ezProcessCommunication::WriteMessages()
{
  ezUInt8* pData = (ezUInt8*)m_pSharedMemory->data();

  ezUInt32* pID = (ezUInt32*)pData;
  *pID = m_uiProcessID;

  pData += sizeof(ezUInt32);

  ezUInt32 uiRemainingSize = (ezUInt32)m_pSharedMemory->size();
  uiRemainingSize -= sizeof(ezUInt32); // process ID
  uiRemainingSize -= sizeof(ezUInt32); // leave some room for the terminator

  {
    EZ_LOCK(m_SendQueueMutex);
    while (!m_MessageSendQueue.IsEmpty())
    {
      ezMemoryStreamStorage& storage = m_MessageSendQueue.PeekFront();

      if (storage.GetStorageSize() + sizeof(ezUInt32) > uiRemainingSize)
        break;

      ezUInt32* pSizeField = (ezUInt32*)pData;

      *pSizeField = storage.GetStorageSize();
      pData += sizeof(ezUInt32);

      ezMemoryUtils::Copy(pData, storage.GetData(), storage.GetStorageSize());
      pData += storage.GetStorageSize();

      uiRemainingSize -= sizeof(ezUInt32);
      uiRemainingSize -= storage.GetStorageSize();

      EZ_ASSERT_DEV(storage.GetRefCount() == 0, "");

      m_MessageSendQueue.PopFront();
    }
  }

  ezUInt32* pTerminator = (ezUInt32*)pData;
  *pTerminator = 0; // terminator, gets overwritten by next message
}

bool ezProcessCommunication::ReadMessages()
{
  ezUInt8* pData = (ezUInt8*)m_pSharedMemory->data();

  ezUInt32* pProcessID = (ezUInt32*)pData;
  pData += sizeof(ezUInt32);

  if (*pProcessID == m_uiProcessID) // if we were the last one to write to the memory, do not read the same data back
    return false;

  ezUInt32* pSizeField = (ezUInt32*)pData;

  while (*pSizeField > 0)
  {
    pData += sizeof(ezUInt32);

    ezMemoryStreamStorage& storage = m_MessageReadQueue.ExpandAndGetRef();
    ezMemoryStreamWriter writer(&storage);

    writer.WriteBytes(pData, *pSizeField);

    pData += *pSizeField;

    pSizeField = (ezUInt32*)pData;
  }

  return true;
}

ezResult ezProcessCommunication::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout)
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function is not thread safe");
  EZ_ASSERT_DEV(m_pWaitForMessageType == nullptr, "Already waiting for another message!");

  m_pWaitForMessageType = pMessageType;

  const ezTime tStart = ezTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    ProcessMessages();

    if (tTimeout != ezTime())
    {
      if (ezTime::Now() - tStart > tTimeout)
      {
        m_pWaitForMessageType = nullptr;
        ezLog::Error("Reached time-out of %.1f seconds while waiting for %s", tTimeout.GetSeconds(), pMessageType->GetTypeName());
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

void ezProcessCommunication::DispatchMessages()
{
  while (!m_MessageReadQueue.IsEmpty())
  {
    // scope to ensure some objects are destroyed before we pop the queue
    {
      ezMemoryStreamStorage& storage = m_MessageReadQueue.PeekFront();
      ezMemoryStreamReader reader(&storage);

      const ezRTTI* pRtti = nullptr;
      ezProcessMessage* pObject = (ezProcessMessage*)ezReflectionSerializer::ReadObjectFromJSON(reader, pRtti);

      if (m_pWaitForMessageType != nullptr && pObject->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
        m_pWaitForMessageType = nullptr;

      EZ_ASSERT_DEV(pRtti != nullptr, "Message Type unknown");
      EZ_ASSERT_DEV(pObject != nullptr, "Object could not be allocated");
      EZ_ASSERT_DEV(pRtti->IsDerivedFrom<ezProcessMessage>(), "Msg base type is invalid");

      Event e;
      e.m_pMessage = pObject;

      m_Events.Broadcast(e);

      pObject->GetDynamicRTTI()->GetAllocator()->Deallocate(pObject);
    }

    m_MessageReadQueue.PopFront();
  }
}

