
EZ_FORCE_INLINE ezGALDevice* ezGALContext::GetDevice() const
{
  return m_pDevice;
}

EZ_FORCE_INLINE void ezGALContext::CountDrawCall()
{
  m_uiDrawCalls++;
}

EZ_FORCE_INLINE void ezGALContext::CountDispatchCall()
{
  m_uiDispatchCalls++;
}

EZ_FORCE_INLINE void ezGALContext::CountStateChange()
{
  m_uiStateChanges++;
}

EZ_FORCE_INLINE void ezGALContext::CountRedundantStateChange()
{
  m_uiRedundantStateChanges++;
}

EZ_FORCE_INLINE void ezGALContext::AssertRenderingThread()
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
}
