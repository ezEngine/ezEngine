
#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Threading/Mutex.h>

#include <poll.h>

class ezIpcChannel;
class ezPipeChannel_linux;

class EZ_FOUNDATION_DLL ezMessageLoop_linux : public ezMessageLoop
{
public:
  ezMessageLoop_linux();
  ~ezMessageLoop_linux();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter) override;

private:
  friend class ezPipeChannel_linux;

  enum class WaitType
  {
    Accept,
    IncomingMessage,
    Connect
  };

  void RegisterWait(ezPipeChannel_linux* pChannel, WaitType type);

private:

  struct WaitInfo
  {
    ezPipeChannel_linux* m_pChannel;
    WaitType m_type;
  };

  // m_waitInfos and m_pollInfos are alway the same size.
  // related information is stored at the same index.
  ezHybridArray<WaitInfo, 16> m_waitInfos;
  ezHybridArray<struct pollfd, 16> m_pollInfos;
  ezMutex m_pollMutex;
  ezAtomicInteger32 m_numPendingPollModifications = 0;
};

#endif
