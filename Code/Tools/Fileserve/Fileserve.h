#pragma once

#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief A stand-alone application for the ezFileServer.
///
/// If EZ_USE_QT is defined, the GUI from the EditorPluginFileserve is used. Otherwise the server runs as a console application.
///
/// If the command line option "-fs_wait_timeout seconds" is specified, the server waits for a limited time for any client to
/// connect and closes automatically, if no connection is established. Once a client connects, this timeout becomes irrelevant.
/// If the command line option "-fs_close_timeout seconds" is specified, the application automatically shuts down when no
/// client is connected anymore and a certain timeout is reached. Once a client connects, the timeout is reset.
/// This timeout has no effect as long as no client has connected.
class ezFileserverApp : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezFileserverApp()
    : ezApplication("Fileserve")
  {
  }

  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual ezApplication::Execution Run() override;
  void FileserverEventHandlerConsole(const ezFileserverEvent& e);
  void FileserverEventHandler(const ezFileserverEvent& e);

  void ShaderMessageHandler(ezFileserveClientContext& ref_ctxt, ezRemoteMessage& ref_msg, ezRemoteInterface& ref_clientChannel, ezDelegate<void(const char*)> logActivity);

  ezUInt32 m_uiSleepCounter = 0;
  ezUInt32 m_uiConnections = 0;
  ezTime m_CloseAppTimeout;
  ezTime m_TimeTillClosing;
};
