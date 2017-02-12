#pragma once

#include <Core/Application/Application.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Fileserver/Fileserver.h>

class ezFileserverApp : public ezApplication
{
public:

  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown();

  virtual ezApplication::ApplicationExecution Run() override;
  void FileserverEventHandlerConsole(const ezFileserverEvent& e);

  ezUInt32 m_uiSleepCounter = 0;
};

