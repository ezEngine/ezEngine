#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/Linux/MessageLoop_linux.h>

#include <Foundation/Communication/Implementation/Linux/PipeChannel_linux.h>
#include <Foundation/Logging/Log.h>

#include <poll.h>

bool ezMessageLoop_linux::WaitForMessages(ezInt32 iTimeout, ezIpcChannel* pFilter)
{
    EZ_ASSERT_ALWAYS(pFilter == nullptr, "Not implemented");

    while(m_numPendingPollModifications > 0)
    {
        ezThreadUtils::YieldTimeSlice();
    }

    ezLock lock{m_pollMutex};
    // TODO: Make poll cancelable
    int result = poll(m_pollInfos.GetData(), m_pollInfos.GetCount(), iTimeout);
    if(result > 0)
    {
        ezUInt32 numEvents = m_pollInfos.GetCount();
        for(ezUInt32 i=0; i < numEvents;)
        {
            WaitInfo& waitInfo = m_waitInfos[i];
            struct pollfd& pollInfo = m_pollInfos[i];
            if(pollInfo.revents != 0)
            {
                switch(waitInfo.m_type)
                {
                    case WaitType::Accept:
                        waitInfo.m_pChannel->AcceptIncomingConnection();
                        m_pollInfos.RemoveAtAndSwap(i);
                        m_waitInfos.RemoveAtAndSwap(i);
                        numEvents--;
                        continue;
                    case WaitType::Connect:
                        waitInfo.m_pChannel->ProcessConnectSuccessfull();
                        m_pollInfos.RemoveAtAndSwap(i);
                        m_waitInfos.RemoveAtAndSwap(i);
                        numEvents--;
                        continue;
                    case WaitType::IncomingMessage:
                        waitInfo.m_pChannel->ProcessIncomingPackages();
                        break;
                }
                pollInfo.revents = 0;
            }
            ++i;
        }
        return true;
    }
    if(result < 0)
    {
        ezLog::Error("ezMessageLoop_linux failed to poll events. Error {}", errno);
    }
    return false;
}

void ezMessageLoop_linux::RegisterWait(ezPipeChannel_linux* pChannel, WaitType type)
{
    short int waitFlags = 0;
    switch(type)
    {
        case WaitType::Accept:
            waitFlags = POLLIN;
            break;
        case WaitType::Connect:
            waitFlags = POLLOUT;
            break;
        case WaitType::IncomingMessage:
            waitFlags = POLLIN;
            break;
    }

    m_numPendingPollModifications.Increment();
    EZ_SCOPE_EXIT(m_numPendingPollModifications.Decrement());
    WakeUp();
    {
        ezLock lock{m_pollMutex};
        m_waitInfos.PushBack({pChannel, type});
        m_pollInfos.PushBack({pChannel->m_serverSocketFd, waitFlags, 0});
    }
}
