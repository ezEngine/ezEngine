#include <PCH.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

ezConeVisualizerAdapter::ezConeVisualizerAdapter()
{
}

ezConeVisualizerAdapter::~ezConeVisualizerAdapter()
{
}

void ezConeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Visualizers are only supported in engine document windows");

  const ezConeVisualizerAttribute* pAttr = static_cast<const ezConeVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Cone, pAttr->m_Color, false, false, true);

  m_Gizmo.SetOwner(pEngineWindow);
  m_Gizmo.SetVisible(true);
}

void ezConeVisualizerAdapter::Update()
{
  const ezConeVisualizerAttribute* pAttr = static_cast<const ezConeVisualizerAttribute*>(m_pVisualizerAttr);

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetAngleProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezAngle>(), "Invalid property bound to ezConeVisualizerAttribute 'angle'");
    m_fAngleScale = ezMath::Tan(value.ConvertTo<ezAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetColorProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezConeVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }

  m_fFinalScale = pAttr->m_fScale;
  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetRadiusProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezConeVisualizerAttribute 'radius'");
    m_fFinalScale *= value.ConvertTo<float>();
  }

  m_Gizmo.SetVisible(m_fAngleScale != 0.0f && m_fFinalScale != 0.0f);
}

void ezConeVisualizerAdapter::UpdateGizmoTransform()
{
  ezMat4 mScale;
  mScale.SetScalingMatrix(ezVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fFinalScale);
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4() * mScale);
}


