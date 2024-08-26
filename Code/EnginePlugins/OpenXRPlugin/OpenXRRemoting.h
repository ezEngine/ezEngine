#pragma once

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
#  include <OpenXRPlugin/Basics.h>
#  include <OpenXRPlugin/OpenXRIncludes.h>

#  include <Foundation/Configuration/Singleton.h>
#  include <GameEngine/XR/XRRemotingInterface.h>


class ezOpenXR;

class ezOpenXRRemoting : public ezXRRemotingInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXRRemoting, ezXRRemotingInterface);

public:
  ezOpenXRRemoting(ezOpenXR* pOpenXR);
  ~ezOpenXRRemoting();

  virtual ezResult Initialize() override;
  virtual ezResult Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual ezResult Connect(const char* szRemoteHostName, uint16_t remotePort, bool bEnableAudio, int iMaxBitrateKbps) override;
  virtual ezResult Disconnect() override;
  virtual ezEnum<ezXRRemotingConnectionState> GetConnectionState() const override;
  virtual ezXRRemotingConnectionEvent& GetConnectionEvent() override;

private:
  friend class ezOpenXR;

  void HandleEvent(const XrEventDataBuffer& event);

private:
  bool m_bInitialized = false;
  ezString m_sPreviousRuntime;
  ezOpenXR* m_pOpenXR = nullptr;
  ezXRRemotingConnectionEvent m_Event;
};
#endif
