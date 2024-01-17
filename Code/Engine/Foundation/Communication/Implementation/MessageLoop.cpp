#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Configuration/Startup.h>

EZ_IMPLEMENT_SINGLETON(ezMessageLoop);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/MessageLoop_Win.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/Platform/Linux/MessageLoop_Linux.h>
#else
#  include <Foundation/Communication/Implementation/MessageLoop_Fallback.h>
#endif

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, MessageLoop)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "TaskSystem",
    "ThreadUtils"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (ezStartup::HasApplicationTag("NoMessageLoop"))
      return;

    #if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
      EZ_DEFAULT_NEW(ezMessageLoop_win);
    #elif EZ_ENABLED(EZ_PLATFORM_LINUX)
      EZ_DEFAULT_NEW(ezMessageLoop_linux);
    #else
      EZ_DEFAULT_NEW(ezMessageLoop_Fallback);
    #endif
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezMessageLoop* pDummy = ezMessageLoop::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

class ezLoopThread : public ezThread
{
public:
  ezLoopThread()
    : ezThread("ezMessageLoopThread")
  {
  }
  ezMessageLoop* m_pRemoteInterface = nullptr;
  virtual ezUInt32 Run() override
  {
    m_pRemoteInterface->RunLoop();
    return 0;
  }
};

ezMessageLoop::ezMessageLoop()
  : m_SingletonRegistrar(this)
{
}

void ezMessageLoop::StartUpdateThread()
{
  EZ_LOCK(m_Mutex);
  if (m_pUpdateThread == nullptr)
  {
    m_pUpdateThread = EZ_DEFAULT_NEW(ezLoopThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void ezMessageLoop::StopUpdateThread()
{
  EZ_LOCK(m_Mutex);
  if (m_pUpdateThread != nullptr)
  {
    m_bShouldQuit = true;
    WakeUp();
    m_pUpdateThread->Join();

    EZ_DEFAULT_DELETE(m_pUpdateThread);
  }
}

void ezMessageLoop::RunLoop()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_ThreadId = ezThreadUtils::GetCurrentThreadID();
#endif

  while (true)
  {
    if (m_bCallTickFunction)
    {
      for (ezIpcChannel* pChannel : m_AllAddedChannels)
      {
        if (pChannel->RequiresRegularTick())
        {
          pChannel->Tick();
        }
      }
    }

    // process all available data until all is processed and we wait for new messages.
    bool didwork = ProcessTasks();
    if (m_bShouldQuit)
      break;

    if (didwork)
      continue;

    didwork |= WaitForMessages(0, nullptr);
    if (m_bShouldQuit)
      break;

    if (didwork)
      continue;

    // wait until we have work again
    WaitForMessages(m_bCallTickFunction ? 50 : -1, nullptr); // timeout 20 times per second, if we need to call the tick function
  }
}

bool ezMessageLoop::ProcessTasks()
{
  {
    EZ_LOCK(m_TasksMutex);
    // Swap out the queues under the lock so we can process them without holding the lock
    m_ConnectQueueTask.Swap(m_ConnectQueue);
    m_SendQueueTask.Swap(m_SendQueue);
    m_DisconnectQueueTask.Swap(m_DisconnectQueue);
  }

  for (ezIpcChannel* pChannel : m_ConnectQueueTask)
  {
    pChannel->InternalConnect();
  }
  for (ezIpcChannel* pChannel : m_SendQueueTask)
  {
    pChannel->InternalSend();
  }
  for (ezIpcChannel* pChannel : m_DisconnectQueueTask)
  {
    pChannel->InternalDisconnect();
  }

  bool bDidWork = !m_ConnectQueueTask.IsEmpty() || !m_SendQueueTask.IsEmpty() || !m_DisconnectQueueTask.IsEmpty();
  m_ConnectQueueTask.Clear();
  m_SendQueueTask.Clear();
  m_DisconnectQueueTask.Clear();
  return bDidWork;
}

void ezMessageLoop::Quit()
{
  m_bShouldQuit = true;
}

void ezMessageLoop::AddChannel(ezIpcChannel* pChannel)
{
  {
    EZ_LOCK(m_TasksMutex);
    m_AllAddedChannels.PushBack(pChannel);

    m_bCallTickFunction = false;
    for (auto pThisChannel : m_AllAddedChannels)
    {
      if (pThisChannel->RequiresRegularTick())
      {
        m_bCallTickFunction = true;
        break;
      }
    }
  }

  StartUpdateThread();
  pChannel->m_pOwner = this;
}

void ezMessageLoop::RemoveChannel(ezIpcChannel* pChannel)
{
  EZ_LOCK(m_TasksMutex);

  m_AllAddedChannels.RemoveAndSwap(pChannel);
  m_ConnectQueue.RemoveAndSwap(pChannel);
  m_DisconnectQueue.RemoveAndSwap(pChannel);
  m_SendQueue.RemoveAndSwap(pChannel);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_MessageLoop);
