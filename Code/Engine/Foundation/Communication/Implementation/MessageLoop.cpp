#include <FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Configuration/Startup.h>

EZ_IMPLEMENT_SINGLETON(ezMessageLoop);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Communication/Implementation/Win/MessageLoop_win.h>
#else
#  include <Foundation/Communication/Implementation/Mobile/MessageLoop_mobile.h>
#endif

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, MessageLoop)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "TaskSystem",
    "ThreadUtils"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    #if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
      EZ_DEFAULT_NEW(ezMessageLoop_win);
    #else
      EZ_DEFAULT_NEW(ezMessageLoop_mobile);
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
  m_threadId = ezThreadUtils::GetCurrentThreadID();
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
  EZ_LOCK(m_TasksMutex);

  // allow to re-queue a connect internally, for this to work, the connect queue musn't be cleared at the end of this
  ezDynamicArray<ezIpcChannel*> conQueue = m_ConnectQueue;
  m_ConnectQueue.Clear();

  for (ezIpcChannel* pChannel : conQueue)
  {
    pChannel->InternalConnect();
  }
  for (ezIpcChannel* pChannel : m_SendQueue)
  {
    pChannel->InternalSend();
  }
  for (ezIpcChannel* pChannel : m_DisconnectQueue)
  {
    pChannel->InternalDisconnect();
  }

  bool bDidWork = !conQueue.IsEmpty() || !m_SendQueue.IsEmpty() || !m_DisconnectQueue.IsEmpty();
  m_SendQueue.Clear();
  m_DisconnectQueue.Clear();
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
  pChannel->AddToMessageLoop(this);
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
