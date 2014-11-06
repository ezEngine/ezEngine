
inline ezView::ezView(const char* szName)
{
  SetName(szName);

  m_pWorld = nullptr;
  m_pRenderPipeline = nullptr;
  m_pLogicCamera = nullptr;
  m_pRenderCamera = nullptr;
}

EZ_FORCE_INLINE const char* ezView::GetName() const
{
  return m_sName.GetString().GetData();
}

EZ_FORCE_INLINE void ezView::SetWorld(const ezWorld* pWorld)
{
  m_pWorld = pWorld;
}

EZ_FORCE_INLINE const ezWorld* ezView::GetWorld() const
{
  return m_pWorld;
}

EZ_FORCE_INLINE void ezView::SetRenderPipeline(ezRenderPipeline* pRenderPipeline)
{
  m_pRenderPipeline = pRenderPipeline;
}

EZ_FORCE_INLINE ezRenderPipeline* ezView::GetRenderPipeline() const
{
  return m_pRenderPipeline;
}

EZ_FORCE_INLINE void ezView::SetLogicCamera(const ezCamera* pCamera)
{
  m_pLogicCamera = pCamera;

  if (m_pRenderCamera == nullptr)
    m_pRenderCamera = m_pLogicCamera;
}

EZ_FORCE_INLINE const ezCamera* ezView::GetLogicCamera() const
{
  return m_pLogicCamera;
}

EZ_FORCE_INLINE void ezView::SetRenderCamera(const ezCamera* pCamera)
{
  m_pRenderCamera = pCamera;

  if (m_pRenderCamera == nullptr)
    m_pRenderCamera = m_pLogicCamera;
}

EZ_FORCE_INLINE const ezCamera* ezView::GetRenderCamera() const
{
  return m_pRenderCamera;
}

EZ_FORCE_INLINE bool ezView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pLogicCamera != nullptr && m_pRenderCamera != nullptr;
}
