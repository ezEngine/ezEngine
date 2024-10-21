#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/EventListener.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace ezRmlUiInternal
{
  static EventListenerInstancer* s_pInstancer;

  void EventListener::ProcessEvent(Rml::Event& ref_event)
  {
    auto context = ref_event.GetTargetElement()->GetContext();
    if (context != nullptr)
    {
      static_cast<ezRmlUiContext*>(context)->ProcessEvent(m_sIdentifier, ref_event);
    }
  }

  void EventListener::OnDetach(Rml::Element* pElement)
  {
    s_pInstancer->ReturnToPool(*this);
  }

  //////////////////////////////////////////////////////////////////////////

  EventListenerInstancer::EventListenerInstancer()
  {
    s_pInstancer = this;
  }

  EventListenerInstancer::~EventListenerInstancer()
  {
    s_pInstancer = nullptr;
  }

  Rml::EventListener* EventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element* pElement)
  {
    EventListener* pListener = nullptr;
    if (m_EventListenerFreelist.IsEmpty())
    {
      pListener = &m_EventListenerPool.ExpandAndGetRef();
      pListener->m_uiIndex = m_EventListenerPool.GetCount() - 1;
    }
    else
    {
      ezUInt32 uiIndex = m_EventListenerFreelist.PeekBack();
      m_EventListenerFreelist.PopBack();

      pListener = &m_EventListenerPool[uiIndex];
    }

    pListener->m_sIdentifier.Assign(value.c_str());
    return pListener;
  }

  void EventListenerInstancer::ReturnToPool(EventListener& ref_listener)
  {
    m_EventListenerFreelist.PushBack(ref_listener.m_uiIndex);
  }

} // namespace ezRmlUiInternal
