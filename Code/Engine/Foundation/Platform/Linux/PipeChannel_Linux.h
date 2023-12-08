#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/IpcChannel.h>

#  include <sys/stat.h>
#  include <sys/types.h>


class EZ_FOUNDATION_DLL ezPipeChannel_linux : public ezIpcChannel
{
public:
  ezPipeChannel_linux(ezStringView sAddress, Mode::Enum mode);
  ~ezPipeChannel_linux();

private:
  friend class ezMessageLoop;
  friend class ezMessageLoop_linux;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  // These are called from MessageLoop_linux on OS events
  void AcceptIncomingConnection();
  void ProcessIncomingPackages();
  void ProcessConnectSuccessfull();

private:
  ezString m_serverSocketPath;
  ezString m_clientSocketPath;
  int m_serverSocketFd = -1;
  int m_clientSocketFd = -1;

  ezUInt8 m_InputBuffer[4096];
  ezUInt64 m_previousSendOffset = 0;
};
#endif
