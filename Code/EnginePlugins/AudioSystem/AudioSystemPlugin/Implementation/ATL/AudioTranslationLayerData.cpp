#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayerData.h>
#include <AudioSystemPlugin/Core/AudioMiddleware.h>

ezATLTrigger::~ezATLTrigger()
{
  if (m_mEvents.IsEmpty() == false)
  {
    for (auto it = m_mEvents.GetIterator(); it.IsValid(); ++it)
    {
      ezATLEvent* event = it.Value();

      if (auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>(); pAudioMiddleware != nullptr)
      {
        pAudioMiddleware->DestroyEventData(event->m_pEventData).IgnoreResult();
      }

      EZ_AUDIOSYSTEM_DELETE(event);
    }

    m_mEvents.Clear();
  }
}

void ezATLTrigger::AttachEvent(const ezAudioSystemDataID uiEventId, ezAudioSystemEventData* const pEventData)
{
  m_mEvents[uiEventId] = EZ_AUDIOSYSTEM_NEW(ezATLEvent, uiEventId, pEventData);
}

void ezATLTrigger::DetachEvent(const ezAudioSystemDataID uiEventId)
{
  EZ_AUDIOSYSTEM_DELETE(m_mEvents[uiEventId]);
  m_mEvents.Remove(uiEventId);
}

ezResult ezATLTrigger::GetEvent(const ezAudioSystemDataID uiEventId, ezAudioSystemEventData*& out_pEventData) const
{
  if (!m_mEvents.Contains(uiEventId))
    return EZ_FAILURE;

  out_pEventData = (*m_mEvents.GetValue(uiEventId))->m_pEventData;
  return EZ_SUCCESS;
}
