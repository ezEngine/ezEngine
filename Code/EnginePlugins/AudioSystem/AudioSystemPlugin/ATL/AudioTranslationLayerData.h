#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>


template <typename T>
using ezATLMapLookup = ezMap<ezAudioSystemDataID, T*, ezCompareHelper<ezAudioSystemDataID>, ezAudioSystemAllocatorWrapper>;

using ezATLEntityLookup = ezATLMapLookup<class ezATLEntity>;
using ezATLListenerLookup = ezATLMapLookup<class ezATLListener>;
using ezATLTriggerLookup = ezATLMapLookup<class ezATLTrigger>;
using ezATLEventLookup = ezATLMapLookup<class ezATLEvent>;
using ezATLRtpcLookup = ezATLMapLookup<class ezATLRtpc>;

class ezATLControl
{
public:
  explicit ezATLControl(const ezAudioSystemDataID uiId)
    : m_uiId(uiId)
  {
  }

  virtual ~ezATLControl() = default;

  [[nodiscard]] virtual ezAudioSystemDataID GetId() const { return m_uiId; }

private:
  const ezAudioSystemDataID m_uiId;
};

class ezATLListener
  : public ezATLControl
{
public:
  explicit ezATLListener(const ezAudioSystemDataID uiId, ezAudioSystemListenerData* const pListenerData = nullptr)
    : ezATLControl(uiId)
    , m_pListenerData(pListenerData)
  {
  }

  ~ezATLListener() override = default;

  ezAudioSystemListenerData* const m_pListenerData;
};

class ezATLEntity final
  : public ezATLControl
{
public:
  explicit ezATLEntity(const ezAudioSystemDataID uiId, ezAudioSystemEntityData* const pEntityData = nullptr)
    : ezATLControl(uiId)
    , m_pEntityData(pEntityData)
  {
  }

  ~ezATLEntity() override = default;

  ezAudioSystemEntityData* const m_pEntityData;
};

class ezATLTrigger final
  : public ezATLControl
{
public:
  explicit ezATLTrigger(const ezAudioSystemDataID uiId, ezAudioSystemTriggerData* const pTriggerData = nullptr)
    : ezATLControl(uiId)
    , m_pTriggerData(pTriggerData)
  {
  }

  ~ezATLTrigger() override;

  /// \brief Attach an event to this trigger. This means the attached event has been triggered by this trigger.
  /// This will ensure that every event will be destroyed when the trigger is destroyed.
  void AttachEvent(ezAudioSystemDataID uiEventId, ezAudioSystemEventData* pEventData);

  /// \brief Detach an event to this trigger. This will also destroy the event.
  void DetachEvent(ezAudioSystemDataID uiEventId);

  /// \brief Get an attached event. This will fail and return nullptr if an event with the given ID is not
  /// attached to this trigger.
  ezResult GetEvent(ezAudioSystemDataID uiEventId, ezAudioSystemEventData*& out_pEventData) const;

  ezAudioSystemTriggerData* const m_pTriggerData;

private:
  ezATLEventLookup m_mEvents;
};

class ezATLEvent final
  : public ezATLControl
{
public:
  explicit ezATLEvent(const ezAudioSystemDataID uiId, ezAudioSystemEventData* const pEventData = nullptr)
    : ezATLControl(uiId)
    , m_pEventData(pEventData)
  {
  }

  ~ezATLEvent() override = default;

  ezAudioSystemEventData* const m_pEventData;
};

class ezATLRtpc final
  : public ezATLControl
{
public:
  explicit ezATLRtpc(const ezAudioSystemDataID uiId, ezAudioSystemRtpcData* const pRtpcData = nullptr)
    : ezATLControl(uiId)
    , m_pRtpcData(pRtpcData)
  {
  }

  ~ezATLRtpc() override = default;

  ezAudioSystemRtpcData* const m_pRtpcData;
};
