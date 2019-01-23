#include <PCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtOrbitCamViewWidget::ezQtOrbitCamViewWidget(ezQtEngineDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig)
    : ezQtEngineViewWidget(nullptr, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pOrbitCameraContext->SetOrbitVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2), true);

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

ezQtOrbitCamViewWidget::~ezQtOrbitCamViewWidget() = default;


void ezQtOrbitCamViewWidget::ConfigureOrbitCameraVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize,
                                                        const ezVec3& vDefaultCameraPosition)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize, vDefaultCameraPosition, true);
}

ezOrbitCameraContext* ezQtOrbitCamViewWidget::GetOrbitCamera()
{
  return m_pOrbitCameraContext.Borrow();
}
