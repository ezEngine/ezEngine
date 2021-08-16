#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtOrbitCamViewWidget::ezQtOrbitCamViewWidget(ezQtEngineDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig, bool bPicking)
  : ezQtEngineViewWidget(nullptr, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pOrbitCameraContext->SetOrbitVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2), true);

  if (bPicking)
  {
    m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts.PushBack(m_pSelectionContext.Borrow());
  }

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

ezQtOrbitCamViewWidget::~ezQtOrbitCamViewWidget() = default;


void ezQtOrbitCamViewWidget::ConfigureOrbitCameraVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vDefaultCameraPosition)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize, vDefaultCameraPosition, true);
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
