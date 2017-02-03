#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Threading/Mutex.h>

struct EZ_PARTICLEPLUGIN_DLL ezParticleEvent
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_vPosition;
  ezVec3 m_vDirection;
  ezVec3 m_vNormal;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEventQueue
{
public:
  EZ_FORCE_INLINE void Clear() { m_Queue.Clear(); }
  EZ_FORCE_INLINE void AddEvent(const ezParticleEvent& e) { m_Queue.PushBack(e); }
  EZ_FORCE_INLINE const ezDeque<ezParticleEvent>& GetAllEvents() const { return m_Queue; }
  EZ_FORCE_INLINE ezUInt32 GetEventTypeHash() const { return m_uiEventTypeHash; }

private:
  friend class ezParticleEventQueueManager;

  bool m_bFree;
  ezUInt32 m_uiEventTypeHash;
  ezDeque<ezParticleEvent> m_Queue;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEventQueueManager
{
public:
  ezParticleEventQueue* CreateEventQueue(ezUInt32 uiEventTypeHash);
  void DestroyEventQueue(ezParticleEventQueue* pQueue);

private:
  mutable ezMutex m_Mutex;
  ezDeque<ezParticleEventQueue> m_AllQueues;
};
