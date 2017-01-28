#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

ezParticleEventQueue* ezParticleEventQueueManager::CreateEventQueue(ezUInt32 uiEventTypeHash)
{
  EZ_LOCK(m_Mutex);

  ezParticleEventQueue* pQueue = nullptr;

  // search for a free queue
  for (auto& queue : m_AllQueues)
  {
    if (queue.m_bFree)
    {
      pQueue = &queue;
      goto found;
    }
  }

  // create a new one, if no free is found
  pQueue = &m_AllQueues.ExpandAndGetRef();

found:

  pQueue->m_bFree = false;
  pQueue->m_uiEventTypeHash = uiEventTypeHash;

  return pQueue;
}

void ezParticleEventQueueManager::DestroyEventQueue(ezParticleEventQueue* pQueue)
{
  EZ_LOCK(m_Mutex);

  pQueue->Clear();
  pQueue->m_bFree = true;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEvent);

