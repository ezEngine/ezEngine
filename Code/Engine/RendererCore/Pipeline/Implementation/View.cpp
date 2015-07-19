#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezView, ezNode, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("RenderTarget0", m_PinRenderTarget0),
    EZ_MEMBER_PROPERTY("RenderTarget1", m_PinRenderTarget1),
    EZ_MEMBER_PROPERTY("RenderTarget2", m_PinRenderTarget2),
    EZ_MEMBER_PROPERTY("RenderTarget3", m_PinRenderTarget3),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezView::ezView()
  : m_ExtractTask("", ezMakeDelegate(&ezView::ExtractData, this))
{
  m_pWorld = nullptr;
  m_pLogicCamera = nullptr;
  m_pRenderCamera = nullptr;

  m_uiLastCameraSettingsModification = 0;
  m_uiLastCameraOrientationModification = 0;
  m_fLastViewportAspectRatio = 1.0f;
}

ezView::~ezView()
{
  
}

void ezView::SetName(const char* szName)
{
  m_sName.Assign(szName);

  ezStringBuilder sb = szName;
  sb.Append(".ExtractData");
  m_ExtractDataProfilingID = ezProfilingSystem::CreateId(sb.GetData());

  sb.Append(" Task");
  m_ExtractTask.SetTaskName(sb);
}

void ezView::ExtractData()
{
  EZ_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  EZ_PROFILE(m_ExtractDataProfilingID);

  m_pRenderPipeline->ExtractData(*this);
}

void ezView::UpdateCachedMatrices() const
{
  const ezCamera* pCamera = GetRenderCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    pCamera->GetViewMatrix(m_Data.m_ViewMatrix);
    m_Data.m_InverseViewMatrix = m_Data.m_ViewMatrix.GetInverse();
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() ||
    m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio = fViewportAspectRatio;

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix);
    m_Data.m_InverseProjectionMatrix = m_Data.m_ProjectionMatrix.GetInverse();
  }

  if (bUpdateVP)
  {
    m_Data.m_ViewProjectionMatrix = m_Data.m_ProjectionMatrix * m_Data.m_ViewMatrix;
    m_Data.m_InverseViewProjectionMatrix = m_Data.m_ViewProjectionMatrix.GetInverse();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);

