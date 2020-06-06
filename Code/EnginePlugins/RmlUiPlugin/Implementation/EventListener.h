#pragma once

#include <Foundation/Strings/HashedString.h>

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

class ezRmlUiContext;

namespace ezRmlUiInternal
{
  class EventListener : public Rml::Core::EventListener
  {
  public:
    virtual void ProcessEvent(Rml::Core::Event& event) override;

    virtual void OnDetach(Rml::Core::Element* element) override;

  private:
    friend class EventListenerInstancer;
    ezHashedString m_sIdentifier;
    ezUInt32 m_uiIndex = 0;
  };

  class EventListenerInstancer : public Rml::Core::EventListenerInstancer
  {
  public:
    EventListenerInstancer();
    ~EventListenerInstancer();

    virtual Rml::Core::EventListener* InstanceEventListener(const Rml::Core::String& value, Rml::Core::Element* element) override;

    void ReturnToPool(EventListener& listener);

  private:
    ezDeque<EventListener> m_EventListenerPool;
    ezDynamicArray<ezUInt32> m_EventListenerFreelist;
  };
} // namespace ezRmlUiInternal
