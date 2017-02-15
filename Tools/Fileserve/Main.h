#pragma once

#include <Core/Application/Application.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Fileserver/Fileserver.h>

/// \brief A stand-alone application for the ezFileServer.
///
/// If EZ_USE_QT is defined, the GUI from the EditorPluginFileserve is used. Otherwise the server runs as a console application.
///
/// If the command line option "-fs_wait_timeout seconds" is specified, the server will wait for a limited time for any client to
/// connect and close automatically, if no connection is established. Once a client connects, the timeout becomes irrelevant.
/// If the command line option "-fs_close_timeout seconds" is specified, the application will automatically shut down when no
/// client is connected anymore and a certain timeout is reached. Once a client connects, the timeout is reset.
/// This timeout has no effect as long as no client has connected.
class ezFileserverApp : public ezApplication
{
public:

  virtual void BeforeCoreStartup() override;
  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown();

  virtual ezApplication::ApplicationExecution Run() override;
  void FileserverEventHandlerConsole(const ezFileserverEvent& e);
  void FileserverEventHandler(const ezFileserverEvent& e);

  ezUInt32 m_uiSleepCounter = 0;
  ezUInt32 m_uiConnections = 0;
  ezTime m_CloseAppTimeout;
  ezTime m_TimeTillClosing;


};

