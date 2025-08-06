#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SplineNodeManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezSplineNodeManipulatorAdapter::ezSplineNodeManipulatorAdapter() = default;
ezSplineNodeManipulatorAdapter::~ezSplineNodeManipulatorAdapter() = default;

void ezSplineNodeManipulatorAdapter::Finalize()
{
  ConfigureGizmos();
}

void ezSplineNodeManipulatorAdapter::Update()
{
  BuildSpline();
  UpdateGizmoTransform();
}

void ezSplineNodeManipulatorAdapter::TangentGizmoEventHandler(const ezGizmoEvent& e)
{
  auto& cp = m_Spline.m_ControlPoints[m_uiNodeIndex];

  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
      m_vLastTangentIn = ezSimdConversion::ToVec3(cp.m_vPosTangentIn);
      m_vLastTangentOut = ezSimdConversion::ToVec3(cp.m_vPosTangentOut);
      BeginTemporaryInteraction();
      break;

    case ezGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case ezGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case ezGizmoEvent::Type::Interaction:
    {
      const ezSplineNodeManipulatorAttribute* pAttr = static_cast<const ezSplineNodeManipulatorAttribute*>(m_pManipulatorAttr);
      
      const float fScale = static_cast<const ezSplineTangentGizmo*>(e.m_pGizmo)->GetScalingResult();
      const ezQuat qRot = static_cast<const ezSplineTangentGizmo*>(e.m_pGizmo)->GetRotationResult();

      if (e.m_pGizmo == &m_TangentInGizmo)
      {
        ezVec3 newTangentIn = (qRot * m_vLastTangentIn) * fScale;

        ChangeProperties(pAttr->GetTangentModeInProperty(), ezSplineTangentMode::Custom, pAttr->GetCustomTangentInProperty(), newTangentIn);
      }
      else
      {
        EZ_ASSERT_DEV(e.m_pGizmo == &m_TangentOutGizmo, "Implementation error");

        ezVec3 newTangentOut = (qRot * m_vLastTangentOut) * fScale;

        ChangeProperties(pAttr->GetTangentModeOutProperty(), ezSplineTangentMode::Custom, pAttr->GetCustomTangentOutProperty(), newTangentOut);
      }
    }
    break;
  }
}

void ezSplineNodeManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_uiNodeIndex == ezInvalidIndex || m_Spline.m_ControlPoints.IsEmpty())
    return;

  auto& cp = m_Spline.m_ControlPoints[m_uiNodeIndex];
  const ezTransform ownerTransform = GetObjectTransform();

  auto MakeGizmoTransform = [&](const ezVec3& forwardDir, const ezVec3& upDir)
  {
    ezVec3 vFwd = forwardDir;
    vFwd.NormalizeIfNotZero().IgnoreResult();

    const ezVec3 vRight = upDir.CrossRH(vFwd).GetNormalized();
    const ezVec3 vUp2 = vFwd.CrossRH(vRight).GetNormalized();

    ezMat3 mLook;
    mLook.SetColumn(0, vFwd);
    mLook.SetColumn(1, vRight);
    mLook.SetColumn(2, vUp2);

    ezTransform t = ezTransform::Make(ezVec3::MakeZero(), ezQuat::MakeFromMat3(mLook));

    return ezTransform::MakeGlobalTransform(ownerTransform, t);
  };

  m_TangentInGizmo.SetTransformation(MakeGizmoTransform(ezSimdConversion::ToVec3(cp.m_vPosTangentIn), ezSimdConversion::ToVec3(cp.m_vUpDirAndRoll)));
  m_TangentOutGizmo.SetTransformation(MakeGizmoTransform(ezSimdConversion::ToVec3(cp.m_vPosTangentOut), ezSimdConversion::ToVec3(cp.m_vUpDirAndRoll)));
}

void ezSplineNodeManipulatorAdapter::BuildSpline()
{
  const ezDocumentObject* pSplineObject = nullptr;
  ezStringView sNodeName;
  {
    const ezDocumentObject* pSplineNodeObject = m_pObject->GetParent();
    sNodeName = pSplineNodeObject->GetTypeAccessor().GetValue("Name").Get<ezString>();

    if (pSplineNodeObject->GetParent() == nullptr)
      return;

    pSplineObject = pSplineNodeObject->GetParent();
  }

  const ezDocumentObject* pSplineComponent = nullptr;
  {
    ezVariantArray componentUuids;
    if (!pSplineObject->GetTypeAccessor().GetValues("Components", componentUuids))
      return;

    for (const auto& v : componentUuids)
    {
      if (v.IsA<ezUuid>())
      {
        pSplineComponent = pSplineObject->GetDocumentObjectManager()->GetObject(v.Get<ezUuid>());
        if (pSplineComponent != nullptr && pSplineComponent->GetType()->GetTypeName() == "ezSplineComponent")
          break;
      }
    }

    if (pSplineComponent == nullptr)
      return;
  }

  ezSplineManipulatorAdapter::BuildSpline(pSplineComponent, "Nodes", "Closed", m_Spline, sNodeName, &m_uiNodeIndex).AssertSuccess();
}

void ezSplineNodeManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);
  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_TangentInGizmo.SetOwner(pEngineWindow, nullptr);
  m_TangentInGizmo.SetVisible(true);
  m_TangentInGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezSplineNodeManipulatorAdapter::TangentGizmoEventHandler, this));

  m_TangentOutGizmo.SetOwner(pEngineWindow, nullptr);
  m_TangentOutGizmo.SetVisible(true);
  m_TangentOutGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezSplineNodeManipulatorAdapter::TangentGizmoEventHandler, this));
}
