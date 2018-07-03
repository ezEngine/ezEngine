#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Foundation/Communication/Implementation/Win/MessageLoop_win.h>
#include <Foundation/Communication/Implementation/Win/PipeChannel_win.h>
#include <Foundation/Communication/IpcChannel.h>

ezMessageLoop_win::ezMessageLoop_win()
{
  m_Port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  EZ_ASSERT_DEBUG(m_Port != INVALID_HANDLE_VALUE, "Failed to create IO completion port!");
}

ezMessageLoop_win::~ezMessageLoop_win()
{
  StopUpdateThread();
  CloseHandle(m_Port);
}


bool ezMessageLoop_win::WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter)
{
  if (iTimeout < 0)
    iTimeout = INFINITE;

  IOItem item;
  if (!MatchCompletedIOItem(pFilter, &item))
  {
    if (!GetIOItem(iTimeout, &item))
      return false;

    if (ProcessInternalIOItem(item))
      return true;
  }

  if (item.pContext->pChannel != NULL)
  {
    if (pFilter != NULL && item.pChannel != pFilter)
    {
      m_CompletedIO.PushBack(item);
    }
    else
    {
      EZ_ASSERT_DEBUG(item.pContext->pChannel == item.pChannel, "");
      static_cast<ezPipeChannel_win*>(item.pChannel)->OnIOCompleted(item.pContext, item.uiBytesTransfered, item.uiError);
    }
  }
  return true;
}

bool ezMessageLoop_win::GetIOItem(ezInt32 iTimeout, IOItem* pItem)
{
  memset(pItem, 0, sizeof(*pItem));
  ULONG_PTR key = 0;
  OVERLAPPED* overlapped = NULL;
  if (!GetQueuedCompletionStatus(m_Port, &pItem->uiBytesTransfered, &key, &overlapped, iTimeout))
  {
    //nothing queued
    if (overlapped == NULL)
      return false;

    pItem->uiError = GetLastError();
    pItem->uiBytesTransfered = 0;
  }

  pItem->pChannel = reinterpret_cast<ezIpcChannel*>(key);
  pItem->pContext = reinterpret_cast<IOContext*>(overlapped);
  return true;
}

bool ezMessageLoop_win::ProcessInternalIOItem(const IOItem& item)
{
  if (reinterpret_cast<ezMessageLoop_win*>(item.pContext) == this &&
      reinterpret_cast<ezMessageLoop_win*>(item.pChannel) == this)
  {
    //internal notification
    EZ_ASSERT_DEBUG(item.uiBytesTransfered == 0, "");
    InterlockedExchange(&m_haveWork, 0);
    return true;
  }
  return false;
}

bool ezMessageLoop_win::MatchCompletedIOItem(ezIpcChannel* pFilter, IOItem* pItem)
{
  for (ezUInt32 i = 0; i < m_CompletedIO.GetCount(); i++)
  {
    if (pFilter == NULL || m_CompletedIO[i].pChannel == pFilter)
    {
      *pItem = m_CompletedIO[i];
      m_CompletedIO.RemoveAt(i);
      return true;
    }
  }
  return false;
}

void ezMessageLoop_win::WakeUp()
{
  if (InterlockedExchange(&m_haveWork, 1))
  {
    //already running
    return;
  }
  //wake up the loop
  BOOL res = PostQueuedCompletionStatus(m_Port, 0, reinterpret_cast<ULONG_PTR>(this),
                                        reinterpret_cast<OVERLAPPED*>(this));
  EZ_ASSERT_DEBUG(res, "Could not PostQueuedCompletionStatus: {0}", ezArgErrorCode(GetLastError()));
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Win_MessageLoop_win);
