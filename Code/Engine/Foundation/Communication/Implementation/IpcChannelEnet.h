#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/IpcChannel.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

class ezRemoteInterface;
class ezRemoteMessage;

class EZ_FOUNDATION_DLL ezIpcChannelEnet : public ezIpcChannel
{
public:
  ezIpcChannelEnet(const char* szAddress, Mode::Enum mode);
  ~ezIpcChannelEnet();

protected:
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;
  virtual bool RequiresRegularTick() { return true; }
  virtual void Tick() override;
  void NetworkMessageHandler(ezRemoteMessage& msg);
  void EnetEventHandler(const ezRemoteEvent& e);

  ezString m_sAddress;
  ezString m_sLastAddress;
  ezTime m_LastConnectAttempt;
  ezUniquePtr<ezRemoteInterface> m_Network;
};

#endif
