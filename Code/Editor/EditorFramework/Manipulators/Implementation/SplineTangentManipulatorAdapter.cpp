#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SplineTangentManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezSplineTangentManipulatorAdapter::ezSplineTangentManipulatorAdapter() = default;
ezSplineTangentManipulatorAdapter::~ezSplineTangentManipulatorAdapter() = default;

void ezSplineTangentManipulatorAdapter::Finalize()
{
  ConfigureGizmos();
}

void ezSplineTangentManipulatorAdapter::Update()
{
  BuildSpline();
  UpdateGizmoTransform();
}

void ezSplineTangentManipulatorAdapter::TangentGizmoEventHandler(const ezGizmoEvent& e)
{
  auto& cp = m_Spline.m_ControlPoints[m_uiNodeIndex];

  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
      m_vLastTangent = m_bIsTangentIn ? ezSimdConversion::ToVec3(cp.m_vPosTangentIn) : ezSimdConversion::ToVec3(cp.m_vPosTangentOut);
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
      ezVec3 newTangent;

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const ezQuat qRot = m_RotateGizmo.GetRotationResult();
        newTangent = qRot * m_vLastTangent;
      }
      else
      {
        EZ_ASSERT_DEV(e.m_pGizmo == &m_ScaleGizmo, "Implementation error");

        const float fScale = m_ScaleGizmo.GetScalingResult().x;
        newTangent = m_vLastTangent * fScale;
      }

      const ezSplineTangentManipulatorAttribute* pAttr = static_cast<const ezSplineTangentManipulatorAttribute*>(m_pManipulatorAttr);
      ChangeProperties(pAttr->GetTangentModeProperty(), ezSplineTangentMode::Custom, pAttr->GetCustomTangentProperty(), newTangent);

      if (CustomTangentsLinked())
      {
        ezStringBuilder sOtherTangentModeProp = pAttr->GetTangentModeProperty();
        ezStringBuilder sOtherCustomTangentProp = pAttr->GetCustomTangentProperty();

        if (m_bIsTangentIn)
        {
          sOtherTangentModeProp.Shrink(0, 2);
          sOtherTangentModeProp.Append("Out");

          sOtherCustomTangentProp.Shrink(0, 2);
          sOtherCustomTangentProp.Append("Out");
        }
        else
        {
          sOtherTangentModeProp.Shrink(0, 3);
          sOtherTangentModeProp.Append("In");

          sOtherCustomTangentProp.Shrink(0, 3);
          sOtherCustomTangentProp.Append("In");
        }

        ChangeProperties(sOtherTangentModeProp, ezSplineTangentMode::Custom, sOtherCustomTangentProp, -newTangent);
      }
    }
    break;
  }
}

void ezSplineTangentManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_uiNodeIndex == ezInvalidIndex || m_Spline.m_ControlPoints.IsEmpty())
    return;

  auto& cp = m_Spline.m_ControlPoints[m_uiNodeIndex];
  const ezTransform ownerTransform = GetObjectTransform();

  const ezVec3 forwardDir = m_bIsTangentIn ? ezSimdConversion::ToVec3(cp.m_vPosTangentIn) : ezSimdConversion::ToVec3(cp.m_vPosTangentOut);
  const ezVec3 upDir = ezSimdConversion::ToVec3(cp.m_vUpDirAndRoll);

  const ezTransform gizmoTransform = [&]()
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
  }();

  m_RotateGizmo.SetTransformation(gizmoTransform);
  m_ScaleGizmo.SetTransformation(gizmoTransform);
}

void ezSplineTangentManipulatorAdapter::BuildSpline()
{
  const ezDocumentObject* pSplineObject = nullptr;
  ezStringView sNodeName;
  {
    const ezDocumentObject* pSplineTangentObject = m_pObject->GetParent();
    sNodeName = pSplineTangentObject->GetTypeAccessor().GetValue("Name").Get<ezString>();

    if (pSplineTangentObject->GetParent() == nullptr)
      return;

    pSplineObject = pSplineTangentObject->GetParent();
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

void ezSplineTangentManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);
  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_RotateGizmo.SetOwner(pEngineWindow, nullptr);
  m_RotateGizmo.EnableAxis(false, true, true);
  m_RotateGizmo.SetVisible(true);
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezSplineTangentManipulatorAdapter::TangentGizmoEventHandler, this));

  m_ScaleGizmo.SetOwner(pEngineWindow, nullptr);
  m_ScaleGizmo.EnableAxis(true, false, false, false);
  m_ScaleGizmo.SetVisible(true);
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezSplineTangentManipulatorAdapter::TangentGizmoEventHandler, this));

  {
    const ezSplineTangentManipulatorAttribute* pAttr = static_cast<const ezSplineTangentManipulatorAttribute*>(m_pManipulatorAttr);
    m_bIsTangentIn = pAttr->GetTangentModeProperty().EndsWith("In");
  }
}

bool ezSplineTangentManipulatorAdapter::CustomTangentsLinked() const
{
  ezVariant linkedVar = m_pObject->GetTypeAccessor().GetValue("LinkCustomTangents");
  return linkedVar.IsA<bool>() && linkedVar.Get<bool>();
}
