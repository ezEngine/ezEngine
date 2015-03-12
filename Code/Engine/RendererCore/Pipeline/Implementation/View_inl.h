
inline ezView::ezView(const char* szName)
{
  SetName(szName);

  m_pWorld = nullptr;
  m_pRenderPipeline = nullptr;
  m_pLogicCamera = nullptr;
  m_pRenderCamera = nullptr;

  m_ViewPortRect = ezRectFloat(0.0f, 0.0f);

  m_uiLastCameraSettingsModification = 0;
  m_uiLastCameraOrientationModification = 0;
  m_fLastViewportAspectRatio = 1.0f;
  m_ViewMatrix.SetIdentity();
  m_InverseViewMatrix.SetIdentity();
  m_ProjectionMatrix.SetIdentity();
  m_InverseProjectionMatrix.SetIdentity();
  m_ViewProjectionMatrix.SetIdentity();
  m_InverseViewProjectionMatrix.SetIdentity();
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

EZ_FORCE_INLINE void ezView::SetViewport(const ezRectFloat& viewport)
{
  m_ViewPortRect = viewport;
}

EZ_FORCE_INLINE const ezRectFloat& ezView::GetViewport() const
{
  return m_ViewPortRect;
}

EZ_FORCE_INLINE bool ezView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pLogicCamera != nullptr && m_pRenderCamera != nullptr && m_ViewPortRect.HasNonZeroArea();
}

EZ_FORCE_INLINE const ezMat4& ezView::GetProjectionMatrix() const 
{ 
  UpdateCachedMatrices(); 
  return m_ProjectionMatrix; 
}

EZ_FORCE_INLINE const ezMat4& ezView::GetInverseProjectionMatrix() const 
{ 
  UpdateCachedMatrices(); 
  return m_InverseProjectionMatrix;
}

EZ_FORCE_INLINE const ezMat4& ezView::GetViewMatrix() const 
{ 
  UpdateCachedMatrices(); 
  return m_ViewMatrix;
}

EZ_FORCE_INLINE const ezMat4& ezView::GetInverseViewMatrix() const 
{ 
  UpdateCachedMatrices(); 
  return m_InverseViewMatrix;
}

EZ_FORCE_INLINE const ezMat4& ezView::GetViewProjectionMatrix() const 
{ 
  UpdateCachedMatrices(); 
  return m_ViewProjectionMatrix;
}

EZ_FORCE_INLINE const ezMat4& ezView::GetInverseViewProjectionMatrix() const 
{ 
  UpdateCachedMatrices(); 
  return m_InverseViewProjectionMatrix;
}
