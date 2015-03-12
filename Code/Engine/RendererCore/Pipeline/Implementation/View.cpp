#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <Foundation/Utilities/GraphicsUtils.h>

void ezView::SetName(const char* szName)
{
  m_sName.Assign(szName);

  ezStringBuilder sb = szName;
  sb.Append(".ExtractData");
  m_ExtractDataProfilingID = ezProfilingSystem::CreateId(sb.GetData());

  sb = szName;
  sb.Append(".Render");
  m_RenderProfilingID = ezProfilingSystem::CreateId(sb.GetData());
}

void ezView::ExtractData()
{
  EZ_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  EZ_PROFILE(m_ExtractDataProfilingID);

  m_pRenderPipeline->ExtractData(*this);
}

void ezView::Render(ezGALContext* pContext)
{
  EZ_ASSERT_DEV(IsValid(), "Cannot render an invalid view");

  EZ_PROFILE(m_RenderProfilingID);

  m_pRenderPipeline->Render(*this, pContext);
}

ezResult ezView::ComputePickingRay(float fScreenPosX, float fScreenPosY, ezVec3& out_RayStartPos, ezVec3& out_RayDir)
{
  UpdateCachedMatrices();

  ezVec3 vScreenPos;
  vScreenPos.x = fScreenPosX;
  vScreenPos.y = 1.0f - fScreenPosY;
  vScreenPos.z = 0.0f;

  return ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix, 0, 0, 1, 1, vScreenPos, out_RayStartPos, &out_RayDir);
}

void ezView::UpdateCachedMatrices() const
{
  EZ_ASSERT_DEV(m_pRenderCamera != nullptr, "Render camera is not set");

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != m_pRenderCamera->GetOrientationModificationCounter())
  {
    bUpdateVP = true;
    m_uiLastCameraOrientationModification = m_pRenderCamera->GetOrientationModificationCounter();

    m_pRenderCamera->GetViewMatrix(m_ViewMatrix);
    m_InverseViewMatrix = m_ViewMatrix.GetInverse();
  }

  if (m_uiLastCameraSettingsModification != m_pRenderCamera->GetSettingsModificationCounter() ||
      m_fLastViewportAspectRatio != m_ViewPortRect.width / m_ViewPortRect.height)
  {
    bUpdateVP = true;
    m_uiLastCameraSettingsModification = m_pRenderCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio = m_ViewPortRect.width / m_ViewPortRect.height;

    m_pRenderCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_ProjectionMatrix);
    m_InverseProjectionMatrix = m_ProjectionMatrix.GetInverse();
  }

  if (bUpdateVP)
  {
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    m_InverseViewProjectionMatrix = m_ViewProjectionMatrix.GetInverse();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);

