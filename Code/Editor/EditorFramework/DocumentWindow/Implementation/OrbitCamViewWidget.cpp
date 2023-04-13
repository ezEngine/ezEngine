#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

ezQtOrbitCamViewWidget::ezQtOrbitCamViewWidget(ezQtEngineDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig, bool bPicking)
  : ezQtEngineViewWidget(nullptr, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);

  if (bPicking)
  {
    m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts.PushBack(m_pSelectionContext.Borrow());
  }

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

ezQtOrbitCamViewWidget::~ezQtOrbitCamViewWidget() = default;


void ezQtOrbitCamViewWidget::ConfigureFixed(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vCamPosition)
{
  m_pOrbitCameraContext->SetDefaultCameraFixed(vCamPosition);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = false;
}

void ezQtOrbitCamViewWidget::ConfigureRelative(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vCamDirection, float fCamDistanceScale)
{
  m_pOrbitCameraContext->SetDefaultCameraRelative(vCamDirection, fCamDistanceScale);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = true;
}

void ezQtOrbitCamViewWidget::SetOrbitVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);

  if (m_bSetDefaultCamPos)
  {
    if (vHalfBoxSize != ezVec3(0.1f))
    {
      // 0.1f is a hard-coded value for the bounding box, in case nothing is available yet
      // not pretty, but somehow we need to know when the first 'proper' bounds are available

      m_bSetDefaultCamPos = false;
      m_pOrbitCameraContext->MoveCameraToDefaultPosition();
    }
  }
}

ezOrbitCameraContext* ezQtOrbitCamViewWidget::GetOrbitCamera()
{
  return m_pOrbitCameraContext.Borrow();
}

void ezQtOrbitCamViewWidget::SyncToEngine()
{
  if (m_pSelectionContext)
  {
    m_pSelectionContext->SetWindowConfig(ezVec2I32(width(), height()));
  }

  ezQtEngineViewWidget::SyncToEngine();
}
