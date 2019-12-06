#include <FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadWithDispatcher.h>

ezThreadWithDispatcher::ezThreadWithDispatcher(const char* szName /*= "ezThreadWithDispatcher"*/, ezUInt32 uiStackSize /*= 128 * 1024*/)
  : ezThread(szName, uiStackSize)
{
}

ezThreadWithDispatcher::~ezThreadWithDispatcher() = default;

void ezThreadWithDispatcher::Dispatch(DispatchFunction&& pDelegate)
{
  EZ_LOCK(m_QueueMutex);
  m_ActiveQueue.PushBack(std::move(pDelegate));
}

void ezThreadWithDispatcher::DispatchQueue()
{
  {
    EZ_LOCK(m_QueueMutex);
    std::swap(m_ActiveQueue, m_CurrentlyBeingDispatchedQueue);
  }

  for (const auto& pDelegate : m_CurrentlyBeingDispatchedQueue)
  {
    pDelegate();
  }

  m_CurrentlyBeingDispatchedQueue.Clear();
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadWithDispatcher);
