#include <PCH.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

ezCapsuleVisualizerAdapter::ezCapsuleVisualizerAdapter()
{
}

ezCapsuleVisualizerAdapter::~ezCapsuleVisualizerAdapter()
{
}

void ezCapsuleVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Visualizers are only supported in engine document windows");

  const ezCapsuleVisualizerAttribute* pAttr = static_cast<const ezCapsuleVisualizerAttribute*>(m_pVisualizerAttr);

  m_Cylinder.Configure(nullptr, ezEngineGizmoHandleType::CylinderZ, pAttr->m_Color, false, false, true);
  m_SphereTop.Configure(nullptr, ezEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, false, false, true);
  m_SphereBottom.Configure(nullptr, ezEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, false, false, true);


  m_Cylinder.SetOwner(pEngineWindow);
  m_SphereTop.SetOwner(pEngineWindow);
  m_SphereBottom.SetOwner(pEngineWindow);

  m_Cylinder.SetVisible(true);
  m_SphereTop.SetVisible(true);
  m_SphereBottom.SetVisible(true);
}

void ezCapsuleVisualizerAdapter::Update()
{
  const ezCapsuleVisualizerAttribute* pAttr = static_cast<const ezCapsuleVisualizerAttribute*>(m_pVisualizerAttr);

  float fRadius = 1.0f;
  float fHeight = 0.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetRadiusProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCapsuleVisualizerAttribute 'radius'");
    fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetHeightProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCapsuleVisualizerAttribute 'height'");
    fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetColorProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezCapsuleVisualizerAttribute 'color'");
    m_SphereTop.SetColor(value.ConvertTo<ezColor>());
    m_SphereBottom.SetColor(value.ConvertTo<ezColor>());
    m_Cylinder.SetColor(value.ConvertTo<ezColor>());
  }

  m_ScaleCylinder.SetScalingMatrix(ezVec3(fRadius, fRadius, fHeight));

  m_ScaleSphereTop.SetScalingMatrix(ezVec3(fRadius));
  m_ScaleSphereTop.SetTranslationVector(ezVec3(0, 0, fHeight * 0.5f));

  m_ScaleSphereBottom.SetScalingMatrix(ezVec3(fRadius, -fRadius, -fRadius));
  m_ScaleSphereBottom.SetTranslationVector(ezVec3(0, 0, -fHeight * 0.5f));

}

void ezCapsuleVisualizerAdapter::UpdateGizmoTransform()
{
  m_SphereTop.SetTransformation(GetObjectTransform().GetAsMat4() * m_ScaleSphereTop);
  m_SphereBottom.SetTransformation(GetObjectTransform().GetAsMat4() * m_ScaleSphereBottom);
  m_Cylinder.SetTransformation(GetObjectTransform().GetAsMat4() * m_ScaleCylinder);
}


