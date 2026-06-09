#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/IpcChannel.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

class ezRemoteInterface;
class ezRemoteMessage;

class EZ_FOUNDATION_DLL ezIpcChannelEnet : public ezIpcChannel
{
public:
  ezIpcChannelEnet(ezStringView sAddress, Mode::Enum mode);
  ~ezIpcChannelEnet();

protected:
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;
  virtual bool RequiresRegularTick() override { return true; }
  virtual void Tick() override;
  void NetworkMessageHandler(ezRemoteMessage& msg);
  void EnetEventHandler(const ezRemoteEvent& e);

  ezString m_sAddress;
  ezTime m_LastConnectAttempt = ezTime::MakeZero();
  ezUniquePtr<ezRemoteInterface> m_pNetwork;
};

#endif
