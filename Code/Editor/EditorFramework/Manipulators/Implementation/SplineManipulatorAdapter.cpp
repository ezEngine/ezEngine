#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezSplineManipulatorAdapter::ezSplineManipulatorAdapter() = default;
ezSplineManipulatorAdapter::~ezSplineManipulatorAdapter() = default;

// static
ezResult ezSplineManipulatorAdapter::BuildSpline(const ezDocumentObject* pSplineComponent, ezStringView sNodesPropertyName, ezStringView sClosedPropertyName, ezSpline& out_spline, ezStringView sNodeName /*= ezStringView()*/, ezUInt32* out_pNodeIndex /*= nullptr*/)
{
  out_spline.m_ControlPoints.Clear();
  out_spline.m_bClosed = false;

  if (pSplineComponent == nullptr)
    return EZ_FAILURE;

  auto& typeAccessor = pSplineComponent->GetTypeAccessor();

  out_spline.m_bClosed = typeAccessor.GetValue(sClosedPropertyName).ConvertTo<bool>();

  ezVariantArray nodeNames;
  if (!typeAccessor.GetValues(sNodesPropertyName, nodeNames))
    return EZ_FAILURE;

  for (auto& v : nodeNames)
  {
    if (!v.IsA<ezHashedString>())
      continue;

    const ezDocumentObject* pNodeComponent = FindSplineNodeComponent(pSplineComponent->GetParent(), v.Get<ezHashedString>());
    if (pNodeComponent == nullptr)
      continue;

    ezSpline::ControlPoint cp;
    if (FillControlPointFromNodeComponent(pNodeComponent, cp).Succeeded())
    {
      out_spline.m_ControlPoints.PushBack(cp);

      if (out_pNodeIndex != nullptr && v.Get<ezHashedString>() == sNodeName)
      {
        *out_pNodeIndex = out_spline.m_ControlPoints.GetCount() - 1;
      }
    }
  }

  out_spline.CalculateUpDirAndAutoTangents();

  return EZ_SUCCESS;
}

// static
const ezDocumentObject* ezSplineManipulatorAdapter::FindSplineNodeComponent(const ezDocumentObject* pSplineObject, ezStringView sNodeName)
{
  ezVariantArray componentUuids;

  for (auto pSplineNodeObject : pSplineObject->GetChildren())
  {
    ezVariant name = pSplineNodeObject->GetTypeAccessor().GetValue("Name");
    if (!name.IsA<ezString>() || name.Get<ezString>() != sNodeName)
      continue;

    if (!pSplineNodeObject->GetTypeAccessor().GetValues("Components", componentUuids))
      continue;

    for (const auto& v : componentUuids)
    {
      if (v.IsA<ezUuid>())
      {
        const ezDocumentObject* pComponent = pSplineObject->GetDocumentObjectManager()->GetObject(v.Get<ezUuid>());
        if (pComponent != nullptr && pComponent->GetType()->GetTypeName() == "ezSplineNodeComponent")
        {
          return pComponent;
        }
      }
    }
  }

  return nullptr;
}

// static
ezResult ezSplineManipulatorAdapter::FillControlPointFromNodeComponent(const ezDocumentObject* pNodeComponent, ezSpline::ControlPoint& out_cp)
{
  {
    ezVariant v = pNodeComponent->GetParent()->GetTypeAccessor().GetValue("LocalPosition");
    if (!v.IsA<ezVec3>())
      return EZ_FAILURE;

    out_cp.m_vPos = ezSimdConversion::ToVec3(v.Get<ezVec3>());
  }

  {
    ezUInt32 uiTangentModeIn = pNodeComponent->GetTypeAccessor().GetValue("TangentModeIn").ConvertTo<ezUInt32>();
    ezVariant v = pNodeComponent->GetTypeAccessor().GetValue("CustomTangentIn");
    if (!v.IsA<ezVec3>())
      return EZ_FAILURE;

    out_cp.SetTangentIn(ezSimdConversion::ToVec3(v.Get<ezVec3>()), static_cast<ezSplineTangentMode::Enum>(uiTangentModeIn));
  }

  {
    ezUInt32 uiTangentModeOut = pNodeComponent->GetTypeAccessor().GetValue("TangentModeOut").ConvertTo<ezUInt32>();
    ezVariant v = pNodeComponent->GetTypeAccessor().GetValue("CustomTangentOut");
    if (!v.IsA<ezVec3>())
      return EZ_FAILURE;

    out_cp.SetTangentOut(ezSimdConversion::ToVec3(v.Get<ezVec3>()), static_cast<ezSplineTangentMode::Enum>(uiTangentModeOut));
  }

  return EZ_SUCCESS;
}

void ezSplineManipulatorAdapter::Finalize()
{
  // Update will be called directly after this, so nothing to do here
}

void ezSplineManipulatorAdapter::Update()
{
  BuildSpline();
  ConfigureGizmos();
}

void ezSplineManipulatorAdapter::ClickGizmoEventHandler(const ezGizmoEvent& e)
{
  int index = -1;

  for (ezUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    if (&m_Gizmos[i] == e.m_pGizmo)
    {
      index = i;
      break;
    }
  }

  EZ_ASSERT_DEBUG(index >= 0, "Gizmo event from unknown gizmo.");
  if (index < 0)
    return;

  auto& gizmo = m_Gizmos[index];
  if (m_Spline.m_bClosed)
  {
    ++index;
  }

  ezStringBuilder sNewNodeName;
  sNewNodeName.SetFormat("{}", index);
  sNewNodeName = MakeUniqueName(sNewNodeName);

  index = ezMath::Min<int>(index, m_Spline.m_ControlPoints.GetCount());

  auto pObjectAcessor = GetObjectAccessor();

  pObjectAcessor->StartTransaction("Add Spline Node");

  // Add a new child game object
  ezUuid gameObjectUuid;
  {
    const ezDocumentObject* pSplineObject = m_pObject->GetParent();
    const ezRTTI* pGameObjectType = ezRTTI::FindTypeByName("ezGameObject");

    if (pObjectAcessor->AddObjectByName(pSplineObject, "Children", index, pGameObjectType, gameObjectUuid).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }

    const ezDocumentObject* pNewObject = pObjectAcessor->GetObject(gameObjectUuid);

    if (pObjectAcessor->SetValueByName(pNewObject, "Name", sNewNodeName.GetView()).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }

    const ezTransform invOwnerTransform = GetObjectTransform().GetInverse();
    const ezVec3 localPos = invOwnerTransform.TransformPosition(gizmo.GetTransformation().m_vPosition);

    if (pObjectAcessor->SetValueByName(pNewObject, "LocalPosition", localPos).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }
  }

  // Add a new ezSplineNodeComponent to the new game object
  ezUuid componentUuid;
  {
    const ezDocumentObject* pNewObject = pObjectAcessor->GetObject(gameObjectUuid);
    const ezRTTI* pSplineNodeType = ezRTTI::FindTypeByName("ezSplineNodeComponent");

    if (pObjectAcessor->AddObjectByName(pNewObject, "Components", 0, pSplineNodeType, componentUuid).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }
  }

  // Finally insert the name into the nodes list
  {
    const ezSplineManipulatorAttribute* pAttr = static_cast<const ezSplineManipulatorAttribute*>(m_pManipulatorAttr);
    if (pObjectAcessor->InsertValueByName(m_pObject, pAttr->GetNodesProperty(), sNewNodeName.GetView(), index).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }
  }

  pObjectAcessor->FinishTransaction();

  Update();
}

void ezSplineManipulatorAdapter::UpdateGizmoTransform()
{
  const ezTransform ownerTransform = GetObjectTransform();
  auto MakeGizmoTransform = [&](const ezVec3& offset)
  {
    ezTransform t = ezTransform::Make(ownerTransform.TransformPosition(offset));
    t.m_vScale.Set(0.1f);
    return t;
  };

  ezUInt32 uiFirstGizmo = 0;
  ezUInt32 uiNumGizmos = m_Gizmos.GetCount();

  if (!m_Spline.m_bClosed)
  {
    if (m_Spline.m_ControlPoints.IsEmpty())
    {
      m_Gizmos.PeekFront().SetTransformation(MakeGizmoTransform(ezVec3(-0.5, 0, 0)));
      m_Gizmos.PeekBack().SetTransformation(MakeGizmoTransform(ezVec3(0.5, 0, 0)));
    }
    else
    {
      auto& cp0 = m_Spline.m_ControlPoints[0];
      auto& cp1 = m_Spline.m_ControlPoints.PeekBack();

      ezVec3 dir0 = ezSimdConversion::ToVec3(cp0.m_vPosTangentIn);
      dir0.NormalizeIfNotZero(ezVec3(-1, 0, 0)).IgnoreResult();
      m_Gizmos.PeekFront().SetTransformation(MakeGizmoTransform(ezSimdConversion::ToVec3(cp0.m_vPos) + dir0));

      ezVec3 dir1 = ezSimdConversion::ToVec3(cp1.m_vPosTangentOut);
      dir1.NormalizeIfNotZero(ezVec3(1, 0, 0)).IgnoreResult();
      m_Gizmos.PeekBack().SetTransformation(MakeGizmoTransform(ezSimdConversion::ToVec3(cp1.m_vPos) + dir1));
    }

    uiFirstGizmo = 1;
    uiNumGizmos = uiNumGizmos - 2;
  }

  for (ezUInt32 i = 0; i < uiNumGizmos; ++i)
  {
    auto& gizmo = m_Gizmos[uiFirstGizmo + i];

    const ezVec3 offset = ezSimdConversion::ToVec3(m_Spline.EvaluatePosition(i + 0.5f));
    gizmo.SetTransformation(MakeGizmoTransform(offset));
  }
}

void ezSplineManipulatorAdapter::BuildSpline()
{
  m_Spline.m_ControlPoints.Clear();
  m_Spline.m_bClosed = false;

  const ezSplineManipulatorAttribute* pAttr = static_cast<const ezSplineManipulatorAttribute*>(m_pManipulatorAttr);
  if (pAttr->GetNodesProperty().IsEmpty() || pAttr->GetClosedProperty().IsEmpty())
    return;

  BuildSpline(m_pObject, pAttr->GetNodesProperty(), pAttr->GetClosedProperty(), m_Spline).AssertSuccess();
}

void ezSplineManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);
  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  const ezUInt32 numGizmos = ezMath::Max(m_Spline.m_ControlPoints.GetCount() + (m_Spline.m_bClosed ? 0 : 1), 2u);
  m_Gizmos.SetCount(numGizmos);

  for (ezUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& click = m_Gizmos[i];
    if (click.IsVisible() && !click.m_GizmoEvents.IsEmpty())
      continue; // already configured

    click.SetOwner(pEngineWindow, nullptr);
    click.SetVisible(true);
    click.SetColor(ezColorScheme::LightUI(ezColorScheme::Red));
    click.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezSplineManipulatorAdapter::ClickGizmoEventHandler, this));
  }

  UpdateGizmoTransform();
}

ezString ezSplineManipulatorAdapter::MakeUniqueName(ezStringView sSuggestedName)
{
  auto IsUniqueName = [&](ezStringView sName)
  {
    const ezSplineManipulatorAttribute* pAttr = static_cast<const ezSplineManipulatorAttribute*>(m_pManipulatorAttr);

    ezVariantArray nodeNames;
    m_pObject->GetTypeAccessor().GetValues(pAttr->GetNodesProperty(), nodeNames);

    for (auto& v : nodeNames)
    {
      if (v.Get<ezHashedString>() == sName)
        return false;
    }

    return true;
  };

  if (IsUniqueName(sSuggestedName))
    return sSuggestedName;

  ezUInt32 uiSuffixIndex = 0;
  ezStringBuilder sCombinedName;

  do
  {
    ++uiSuffixIndex;
    sCombinedName.SetFormat("{}.{}", sSuggestedName, uiSuffixIndex);
  } while (!IsUniqueName(sCombinedName));

  return sCombinedName;
}
