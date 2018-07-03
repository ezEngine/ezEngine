#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/Implementation/Win/MessageLoop_win.h>
#include <Foundation/Communication/Implementation/Win/PipeChannel_win.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

ezPipeChannel_win::State::State(ezPipeChannel_win* pChannel) : IsPending(false)
{
  memset(&Context.Overlapped, 0, sizeof(Context.Overlapped));
  Context.pChannel = pChannel;
  IsPending = false;
}

ezPipeChannel_win::State::~State()
{
}

ezPipeChannel_win::ezPipeChannel_win(const char* szAddress, Mode::Enum mode)
    : ezIpcChannel(szAddress, mode), m_InputState(this), m_OutputState(this)
{
  CreatePipe(szAddress);
  m_pOwner->AddChannel(this);
}

ezPipeChannel_win::~ezPipeChannel_win()
{
  if (m_PipeHandle != INVALID_HANDLE_VALUE)
  {
    Disconnect();
  }
  while (m_Connected)
  {
    ezThreadUtils::Sleep(ezTime::Milliseconds(10));
  }

  m_pOwner->RemoveChannel(this);
}

bool ezPipeChannel_win::CreatePipe(const char* szAddress)
{
  ezStringBuilder sPipename("\\\\.\\pipe\\", szAddress);

  if (m_Mode == Mode::Server)
  {
    SECURITY_ATTRIBUTES attributes = {0};
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = NULL;
    attributes.bInheritHandle = FALSE;

    m_PipeHandle = CreateNamedPipeW(ezStringWChar(sPipename).GetData(),
                                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                    1,
                                    BUFFER_SIZE,
                                    BUFFER_SIZE,
                                    5000,
                                    &attributes);
  }
  else
  {
    m_PipeHandle = CreateFileW(ezStringWChar(sPipename).GetData(),
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION | FILE_FLAG_OVERLAPPED,
                               NULL);
  }

  if (m_PipeHandle == INVALID_HANDLE_VALUE)
  {
    ezLog::Error("Could not create named pipe: {0}", ezArgErrorCode(GetLastError()));
    return false;
  }

  return true;
}

void ezPipeChannel_win::AddToMessageLoop(ezMessageLoop* pMsgLoop)
{
  if (m_PipeHandle != INVALID_HANDLE_VALUE)
  {
    ezMessageLoop_win* pMsgLoopWin = static_cast<ezMessageLoop_win*>(pMsgLoop);

    ULONG_PTR key = reinterpret_cast<ULONG_PTR>(this);
    HANDLE port = CreateIoCompletionPort(m_PipeHandle, pMsgLoopWin->GetPort(), key, 1);
    EZ_ASSERT_DEBUG(pMsgLoopWin->GetPort() == port, "Failed to CreateIoCompletionPort: {0}", ezArgErrorCode(GetLastError()));
  }
}

void ezPipeChannel_win::InternalConnect()
{
  if (m_PipeHandle == INVALID_HANDLE_VALUE)
    return;
  if (m_Connected)
    return;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (m_ThreadId == 0)
    m_ThreadId = ezThreadUtils::GetCurrentThreadID();
#endif

  if (m_Mode == Mode::Server)
  {
    ProcessConnection();
  }
  else
  {
    m_Connected = true;
  }

  if (!m_InputState.IsPending)
  {
    OnIOCompleted(&m_InputState.Context, 0, 0);
  }

  if (m_Connected)
  {
    ProcessOutgoingMessages(0);
  }

  m_Events.Broadcast(ezIpcChannelEvent(m_Mode == Mode::Client ? ezIpcChannelEvent::ConnectedToServer : ezIpcChannelEvent::ConnectedToClient, this));
  return;
}

void ezPipeChannel_win::InternalDisconnect()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (m_ThreadId != 0)
    EZ_ASSERT_DEBUG(m_ThreadId == ezThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
#endif
  if (m_InputState.IsPending || m_OutputState.IsPending)
  {
    CancelIo(m_PipeHandle);
  }

  if (m_PipeHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_PipeHandle);
    m_PipeHandle = INVALID_HANDLE_VALUE;
  }

  while (m_InputState.IsPending || m_OutputState.IsPending)
  {
    FlushPendingOperations();
  }

  {
    EZ_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
    m_Connected = false;
  }

  m_Events.Broadcast(ezIpcChannelEvent(m_Mode == Mode::Client ? ezIpcChannelEvent::DisconnectedFromServer : ezIpcChannelEvent::DisconnectedFromClient, this));
  // Raise in case another thread is waiting for new messages (as we would sleep forever otherwise).
  m_IncomingMessages.RaiseSignal();
}

void ezPipeChannel_win::InternalSend()
{
  if (!m_OutputState.IsPending && m_Connected)
  {
    ProcessOutgoingMessages(0);
  }
}


bool ezPipeChannel_win::NeedWakeup() const
{
  return m_OutputState.IsPending == 0;
}

bool ezPipeChannel_win::ProcessConnection()
{
  EZ_ASSERT_DEBUG(m_ThreadId == ezThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  if (m_InputState.IsPending)
    m_InputState.IsPending = false;

  BOOL res = ConnectNamedPipe(m_PipeHandle, &m_InputState.Context.Overlapped);
  if (res)
  {
    //EZ_REPORT_FAILURE
    return false;
  }

  ezUInt32 error = GetLastError();
  switch (error)
  {
    case ERROR_IO_PENDING:
      m_InputState.IsPending = true;
      break;
    case ERROR_PIPE_CONNECTED:
      m_Connected = true;
      break;
    case ERROR_NO_DATA:
      return false;
    default:
      ezLog::Error("Could not connect to pipe (Error code: {0})", ezArgErrorCode(error));
      return false;
  }

  return true;
}

bool ezPipeChannel_win::ProcessIncomingMessages(DWORD uiBytesRead)
{
  EZ_ASSERT_DEBUG(m_ThreadId == ezThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  if (m_InputState.IsPending)
  {
    m_InputState.IsPending = false;
    if (uiBytesRead == 0)
      return false;
  }

  while (true)
  {
    if (uiBytesRead == 0)
    {
      if (m_PipeHandle == INVALID_HANDLE_VALUE)
        return false;

      BOOL res = ReadFile(m_PipeHandle, m_InputBuffer, BUFFER_SIZE, &uiBytesRead,
                          &m_InputState.Context.Overlapped);

      if (!res)
      {
        ezUInt32 error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
          m_InputState.IsPending = true;
          return true;
        }
        if (m_Mode == Mode::Server)
        {
          // only log when in server mode, otherwise this can result in an endless recursion
          ezLog::Error("Read from pipe failed: {0}", ezArgErrorCode(error));
        }
        return false;
      }
      m_InputState.IsPending = true;
      return true;
    }

    EZ_ASSERT_DEBUG(uiBytesRead != 0, "We really should have data at this point.");
    ReceiveMessageData(ezArrayPtr<ezUInt8>(m_InputBuffer, uiBytesRead));
    uiBytesRead = 0;
  }
  return true;
}

bool ezPipeChannel_win::ProcessOutgoingMessages(DWORD uiBytesWritten)
{
  EZ_ASSERT_DEBUG(m_Connected, "Must be connected to process outgoing messages.");
  EZ_ASSERT_DEBUG(m_ThreadId == ezThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");

  if (m_OutputState.IsPending)
  {
    if (uiBytesWritten == 0)
    {
      // Don't reset isPending right away as we want to use it to decide
      // whether we need to wake up the worker thread again.
      ezLog::Error("pipe error: {0}", ezArgErrorCode(GetLastError()));
      m_OutputState.IsPending = false;
      return false;
    }

    EZ_LOCK(m_OutputQueueMutex);
    //message was send
    m_OutputQueue.PopFront();
  }

  if (m_PipeHandle == INVALID_HANDLE_VALUE)
  {
    m_OutputState.IsPending = false;
    return false;
  }
  const ezMemoryStreamStorage* storage = nullptr;
  {
    EZ_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      m_OutputState.IsPending = false;
      return true;
    }
    storage = &m_OutputQueue.PeekFront();
  }

  BOOL res = WriteFile(m_PipeHandle, storage->GetData(), storage->GetStorageSize(), &uiBytesWritten,
                       &m_OutputState.Context.Overlapped);

  if (!res)
  {
    ezUInt32 error = GetLastError();
    if (error == ERROR_IO_PENDING)
    {
      m_OutputState.IsPending = true;
      return true;
    }
    ezLog::Error("Write to pipe failed: {0}", ezArgErrorCode(error));
    return false;
  }

  m_OutputState.IsPending = true;
  return true;
}

void ezPipeChannel_win::OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError)
{
  EZ_ASSERT_DEBUG(m_ThreadId == ezThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  bool bRes = true;
  if (pContext == &m_InputState.Context)
  {
    if (!m_Connected)
    {
      if (!ProcessConnection())
        return;

      bool bHasOutput = false;
      {
        EZ_LOCK(m_OutputQueueMutex);
        bHasOutput = !m_OutputQueue.IsEmpty();
      }

      if (bHasOutput && m_OutputState.IsPending == 0)
        ProcessOutgoingMessages(0);
      if (m_InputState.IsPending)
        return;
    }
    bRes = ProcessIncomingMessages(uiBytesTransfered);
  }
  else
  {
    EZ_ASSERT_DEBUG(pContext == &m_OutputState.Context, "");
    bRes = ProcessOutgoingMessages(uiBytesTransfered);
  }
  if (!bRes && m_PipeHandle != INVALID_HANDLE_VALUE)
  {
    InternalDisconnect();
  }
}
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Win_PipeChannel_win);
