#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Foundation/Basics.h>
#include <Foundation/Communication/IpcChannel.h>

struct IOContext
{
  OVERLAPPED Overlapped;///< Must be first field in class so we can do a reinterpret cast from *Overlapped to *IOContext.
  ezIpcChannel* pChannel; ///< Owner of this IOContext.
};

class EZ_FOUNDATION_DLL ezPipeChannel_win : public ezIpcChannel
{
public:
  ezPipeChannel_win(const char* szAddress, Mode::Enum mode);
  ~ezPipeChannel_win();

private:
  friend class ezMessageLoop;
  friend class ezMessageLoop_win;

  bool CreatePipe(const char* szAddress);

  virtual void AddToMessageLoop(ezMessageLoop* pMsgLoop) override;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  bool ProcessConnection();
  bool ProcessIncomingMessages(DWORD uiBytesRead);
  bool ProcessOutgoingMessages(DWORD uiBytesWritten);


protected:
  void OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError);

private:
  struct State
  {
    explicit State(ezPipeChannel_win* pChannel);
    ~State();
    IOContext Context;
    ezAtomicInteger32 IsPending = false; ///< Whether an async operation is in process.
  };

  enum Constants
  {
    BUFFER_SIZE = 4096,
  };

  // Shared data
  State m_InputState;
  State m_OutputState;

  // Setup in ctor
  HANDLE m_PipeHandle = INVALID_HANDLE_VALUE;

  // Only accessed from worker thread
  ezUInt8 m_InputBuffer[BUFFER_SIZE];
};

#endif

