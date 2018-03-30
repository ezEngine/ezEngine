#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>

class ezProcessMessage;
class ezIpcChannel;
class ezLoopThread;

/// \brief Internal sub-system used by ezIpcChannel.
///
/// This sub-system creates a background thread as soon as the first ezIpcChannel
/// is added to it. This class should never be needed to be accessed outside
/// of ezIpcChannel implementations.
class EZ_FOUNDATION_DLL ezMessageLoop
{
  EZ_DECLARE_SINGLETON(ezMessageLoop);
public:
  ezMessageLoop();
  virtual ~ezMessageLoop() {};

  /// \brief Needs to be called by newly created channels' constructors.
  void AddChannel(ezIpcChannel* pChannel);

  void RemoveChannel(ezIpcChannel* pChannel);

protected:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, MessageLoop);
  friend class ezLoopThread;
  friend class ezIpcChannel;

  void StartUpdateThread();
  void StopUpdateThread();
  void RunLoop();
  bool ProcessTasks();
  void Quit();

  /// \brief Wake up the message loop when new work comes in.
  virtual void WakeUp() = 0;
  /// \brief Waits until a new message has been processed (sent, received).
  /// \param timeout If negative, wait indefinitely.
  /// \param pFilter If not null, wait for a message for the specific channel.
  /// \return Returns whether a message was received or the timeout was reached.
  virtual bool WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter) = 0;

  ezThreadID m_threadId = 0;
  mutable ezMutex m_Mutex;
  bool m_bShouldQuit = false;
  bool m_bCallTickFunction = false;
  class ezLoopThread* m_pUpdateThread = nullptr;

  ezMutex m_TasksMutex;
  ezDynamicArray<ezIpcChannel*> m_ConnectQueue;
  ezDynamicArray<ezIpcChannel*> m_DisconnectQueue;
  ezDynamicArray<ezIpcChannel*> m_SendQueue;
  ezDynamicArray<ezIpcChannel*> m_AllAddedChannels;
};

