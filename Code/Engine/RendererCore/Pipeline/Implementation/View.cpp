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

  m_pRenderPipeline->ExtractData(*m_pWorld, *m_pLogicCamera);
}

void ezView::Render(ezGALContext* pContext)
{
  EZ_ASSERT_DEV(IsValid(), "Cannot render an invalid view");

  EZ_PROFILE(m_RenderProfilingID);

  m_pRenderPipeline->Render(*m_pRenderCamera, pContext);
}

ezResult ezView::GetPickingRay(float fScreenPosX, float fScreenPosY, ezVec3& out_RayStartPos, ezVec3& out_RayDir)
{
  ezVec3 vScreenPos;
  vScreenPos.x = fScreenPosX;
  vScreenPos.y = 1.0f - fScreenPosY;
  vScreenPos.z = 0.0f;

  const ezRectFloat vp = m_pRenderPipeline->GetViewport();

  /// \todo I don't know whether we can cache this somewhere
  /// ezRenderPipeline only updates its matrices upon Render, which might not be sufficient (ie. we might call this code before any rendering from that view is done)
  /// and the camera could change in between
  ezMat4 mProj, mView, mViewProj, mInverseViewProj;
  m_pRenderCamera->GetProjectionMatrix(vp.width / vp.height, mProj); // always use the render camera, not the logic camera
  m_pRenderCamera->GetViewMatrix(mView);
  mViewProj = mProj * mView;

  mInverseViewProj = mViewProj.GetInverse();

  return ezGraphicsUtils::ConvertScreenPosToWorldPos(mInverseViewProj, 0, 0, 1, 1, vScreenPos, out_RayStartPos, &out_RayDir);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);

