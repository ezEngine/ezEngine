#pragma once

#include <Core/Application/Application.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Types/UniquePtr.h>

class ezFileserverApp : public ezApplication
{
public:
  ezFileserverApp();

  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown();

  virtual ezApplication::ApplicationExecution Run() override;

  ezUniquePtr<ezNetworkInterface> m_Network;
};

