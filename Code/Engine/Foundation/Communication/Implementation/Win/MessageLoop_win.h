#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Foundation/Basics.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>

class ezIpcChannel;
struct IOContext;

class EZ_FOUNDATION_DLL ezMessageLoop_win : public ezMessageLoop
{
public:
  struct IOItem
  {
    EZ_DECLARE_POD_TYPE();

    ezIpcChannel* pChannel;
    IOContext* pContext;
    DWORD uiBytesTransfered;
    DWORD uiError;
  };

public:
  ezMessageLoop_win();
  ~ezMessageLoop_win();

  HANDLE GetPort() const { return m_Port; }

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter) override;

  bool GetIOItem(ezInt32 iTimeout, IOItem* pItem);
  bool ProcessInternalIOItem(const IOItem& item);
  bool MatchCompletedIOItem(ezIpcChannel* pFilter, IOItem* pItem);

private:
  ezDynamicArray<IOItem> m_CompletedIO;
  LONG m_haveWork = 0;
  HANDLE m_Port = INVALID_HANDLE_VALUE;
};

#endif

