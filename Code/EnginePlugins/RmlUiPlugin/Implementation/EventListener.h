#pragma once

#include <Foundation/Strings/HashedString.h>

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

class ezRmlUiContext;

namespace ezRmlUiInternal
{
  class EventListener final : public Rml::EventListener
  {
  public:
    virtual void ProcessEvent(Rml::Event& event) override;

    virtual void OnDetach(Rml::Element* element) override;

  private:
    friend class EventListenerInstancer;
    ezHashedString m_sIdentifier;
    ezUInt32 m_uiIndex = 0;
  };

  class EventListenerInstancer final : public Rml::EventListenerInstancer
  {
  public:
    EventListenerInstancer();
    ~EventListenerInstancer();

    virtual Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;

    void ReturnToPool(EventListener& listener);

  private:
    ezDeque<EventListener> m_EventListenerPool;
    ezDynamicArray<ezUInt32> m_EventListenerFreelist;
  };
} // namespace ezRmlUiInternal
