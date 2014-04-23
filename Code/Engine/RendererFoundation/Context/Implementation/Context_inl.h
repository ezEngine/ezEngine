
inline void ezGALContext::CountDrawCall()
{
  m_uiDrawCalls++;
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
