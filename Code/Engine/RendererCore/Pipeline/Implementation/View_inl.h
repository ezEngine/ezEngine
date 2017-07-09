
EZ_ALWAYS_INLINE ezViewHandle ezView::GetHandle() const
{
  return ezViewHandle(m_InternalId);
}

EZ_ALWAYS_INLINE const char* ezView::GetName() const
{
  return m_sName.GetString();
}

EZ_ALWAYS_INLINE void ezView::SetWorld(ezWorld* pWorld)
{
  m_pWorld = pWorld;
}

EZ_ALWAYS_INLINE ezWorld* ezView::GetWorld()
{
  return m_pWorld;
}

EZ_ALWAYS_INLINE const ezWorld* ezView::GetWorld() const
{
  return m_pWorld;
}

EZ_ALWAYS_INLINE const ezGALRenderTagetSetup& ezView::GetRenderTargetSetup() const
{
  return m_RenderTargetSetup;
}

EZ_ALWAYS_INLINE void ezView::SetCamera(ezCamera* pCamera)
{
  m_pCamera = pCamera;
}

EZ_ALWAYS_INLINE ezCamera* ezView::GetCamera()
{
  return m_pCamera;
}

EZ_ALWAYS_INLINE const ezCamera* ezView::GetCamera() const
{
  return m_pCamera;
}

EZ_ALWAYS_INLINE void ezView::SetCullingCamera(ezCamera* pCamera)
{
  m_pCullingCamera = pCamera;
}

EZ_ALWAYS_INLINE ezCamera* ezView::GetCullingCamera()
{
  return m_pCullingCamera != nullptr ? m_pCullingCamera : m_pCamera;
}

EZ_ALWAYS_INLINE const ezCamera* ezView::GetCullingCamera() const
{
  return m_pCullingCamera != nullptr ? m_pCullingCamera : m_pCamera;
}

EZ_ALWAYS_INLINE const ezRectFloat& ezView::GetViewport() const
{
  return m_Data.m_ViewPortRect;
}

EZ_ALWAYS_INLINE const ezViewData& ezView::GetData() const
{
  UpdateCachedMatrices();
  return m_Data;
}

EZ_FORCE_INLINE bool ezView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

EZ_ALWAYS_INLINE ezTask* ezView::GetExtractTask()
{
  return &m_ExtractTask;
}

EZ_FORCE_INLINE ezResult ezView::ComputePickingRay(float fScreenPosX, float fScreenPosY, ezVec3& out_RayStartPos, ezVec3& out_RayDir) const
{
  UpdateCachedMatrices();
  return m_Data.ComputePickingRay(fScreenPosX, fScreenPosY, out_RayStartPos, out_RayDir);
}

EZ_FORCE_INLINE ezResult ezView::ComputeScreenSpacePos(const ezVec3& vPoint, ezVec3& out_vScreenPos) const
{
  UpdateCachedMatrices();
  return m_Data.ComputeScreenSpacePos(vPoint, out_vScreenPos);
}

EZ_ALWAYS_INLINE const ezMat4& ezView::GetProjectionMatrix(ezCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ProjectionMatrix[static_cast<int>(eye)];
}

EZ_ALWAYS_INLINE const ezMat4& ezView::GetInverseProjectionMatrix(ezCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseProjectionMatrix[static_cast<int>(eye)];
}

EZ_ALWAYS_INLINE const ezMat4& ezView::GetViewMatrix(ezCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewMatrix[static_cast<int>(eye)];
}

EZ_ALWAYS_INLINE const ezMat4& ezView::GetInverseViewMatrix(ezCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewMatrix[static_cast<int>(eye)];
}

EZ_ALWAYS_INLINE const ezMat4& ezView::GetViewProjectionMatrix(ezCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewProjectionMatrix[static_cast<int>(eye)];
}

EZ_ALWAYS_INLINE const ezMat4& ezView::GetInverseViewProjectionMatrix(ezCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewProjectionMatrix[static_cast<int>(eye)];
}
