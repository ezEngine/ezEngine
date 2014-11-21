#include <PCH.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Logging/Log.h>

ezProcessCommunication::ezProcessCommunication()
{
  m_pClientProcess = nullptr;
  m_pSharedMemory = nullptr;
  m_uiProcessID = 0;
}

ezResult ezProcessCommunication::StartClientProcess(const char* szProcess, ezUInt32 uiMemSize)
{
  EZ_LOG_BLOCK("ezProcessCommunication::StartClientProcess");

  EZ_ASSERT(m_pSharedMemory == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

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
  ezMemoryUtils::ZeroFill<ezUInt8>((ezUInt8*) m_pSharedMemory->data(), m_pSharedMemory->size());
  EZ_VERIFY(m_pSharedMemory->unlock(), "Implementation error?");

  ezStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = ezOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  QStringList arguments;
  arguments << "-IPC";
  arguments << QLatin1String(sMemName.GetData());

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
  EZ_ASSERT(m_pSharedMemory == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_uiProcessID = 0x0A0A0A0A;

  if (ezStringUtils::IsNullOrEmpty(ezCommandLineUtils::GetInstance()->GetStringOption("-IPC")))
  {
    EZ_REPORT_FAILURE("Command Line does not contain -IPC parameter");
    return EZ_FAILURE;
  }

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
  m_uiProcessID = 0;

  delete m_pClientProcess;
  m_pClientProcess = nullptr;

  delete m_pSharedMemory;
  m_pSharedMemory = nullptr;

  m_MessageSendQueue.Clear();
  m_MessageReadQueue.Clear();
}

bool ezProcessCommunication::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void ezProcessCommunication::SendMessage(ezProcessMessage* pMessage)
{
  ezMemoryStreamStorage& storage = m_MessageSendQueue.ExpandAndGetRef();
  ezMemoryStreamWriter writer(&storage);

  ezReflectionUtils::WriteObjectToJSON(writer, pMessage->GetDynamicRTTI(), pMessage);
}

void ezProcessCommunication::ProcessMessages()
{
  if (!m_pSharedMemory)
    return;

  EZ_VERIFY(m_pSharedMemory->lock(), "Implementation error?");

  ezUInt8* pData = (ezUInt8*) m_pSharedMemory->data();

  if (ReadMessages())
  {
    WriteMessages();
  }

  EZ_VERIFY(m_pSharedMemory->unlock(), "Implementation error?");

  DispatchMessages();
}

void ezProcessCommunication::WriteMessages()
{
  ezUInt8* pData = (ezUInt8*) m_pSharedMemory->data();

  ezUInt32* pID = (ezUInt32*) pData;
  *pID = m_uiProcessID;

  pData += sizeof(ezUInt32);

  ezUInt32 uiRemainingSize = (ezUInt32) m_pSharedMemory->size();
  uiRemainingSize -= sizeof(ezUInt32); // process ID
  uiRemainingSize -= sizeof(ezUInt32); // leave some room for the terminator

  while (!m_MessageSendQueue.IsEmpty())
  {
    ezMemoryStreamStorage& storage = m_MessageSendQueue.PeekFront();

    if (storage.GetStorageSize() + sizeof(ezUInt32) > uiRemainingSize)
      break;

    ezUInt32* pSizeField = (ezUInt32*) pData;

    *pSizeField = storage.GetStorageSize();
    pData += sizeof(ezUInt32);

    ezMemoryUtils::Copy(pData, storage.GetData(), storage.GetStorageSize());
    pData += storage.GetStorageSize();

    uiRemainingSize -= sizeof(ezUInt32);
    uiRemainingSize -= storage.GetStorageSize();
    m_MessageSendQueue.PopFront();
  }

  ezUInt32* pTerminator = (ezUInt32*) pData;
  *pTerminator = 0; // terminator, gets overwritten by next message
}

bool ezProcessCommunication::ReadMessages()
{
  ezUInt8* pData = (ezUInt8*) m_pSharedMemory->data();

  ezUInt32* pProcessID = (ezUInt32*) pData;
  pData += sizeof(ezUInt32);

  if (*pProcessID == m_uiProcessID) // if we were the last one to write to the memory, do not read the same data back
    return false;

  ezUInt32* pSizeField = (ezUInt32*) pData;

  while (*pSizeField > 0)
  {
    pData += sizeof(ezUInt32);

    ezMemoryStreamStorage& storage = m_MessageReadQueue.ExpandAndGetRef();
    ezMemoryStreamWriter writer(&storage);

    writer.WriteBytes(pData, *pSizeField);

    pData += *pSizeField;

    pSizeField = (ezUInt32*) pData;
  }

  return true;
}

void ezProcessCommunication::DispatchMessages()
{
  while (!m_MessageReadQueue.IsEmpty())
  {
    {
      ezMemoryStreamStorage& storage = m_MessageReadQueue.PeekFront();
      ezMemoryStreamReader reader(&storage);

      const ezRTTI* pRtti = nullptr;
      ezProcessMessage* pObject = (ezProcessMessage*) ezReflectionUtils::ReadObjectFromJSON(reader, pRtti);

      EZ_ASSERT(pRtti != nullptr, "Message Type unknown");
      EZ_ASSERT(pObject != nullptr, "Object could not be allocated");
      EZ_ASSERT(pRtti->IsDerivedFrom<ezProcessMessage>(), "Msg base type is invalid");

      Event e;
      e.m_pMessage = pObject;

      m_Events.Broadcast(e);

      pObject->GetDynamicRTTI()->GetAllocator()->Deallocate(pObject);
    }

    m_MessageReadQueue.PopFront();
  }
}

