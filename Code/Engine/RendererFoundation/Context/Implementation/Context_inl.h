
inline void ezGALContext::CountDrawCall()
{
  m_uiDrawCalls++;

  ezGALContextEvent ed;
  ed.m_pContext = this;
  ed.m_EventType = ezGALContextEvent::AfterDrawcall;
  s_ContextEvents.Broadcast(ed);
}

inline void ezGALContext::CountDispatchCall()
{
  m_uiDispatchCalls++;
}

inline void ezGALContext::CountStateChange()
{
  m_uiStateChanges++;
}

inline void ezGALContext::CountRedundantStateChange()
{
  m_uiRedundantStateChanges++;
}

inline void ezGALContext::AssertRenderingThread()
{
  /// \todo
}
