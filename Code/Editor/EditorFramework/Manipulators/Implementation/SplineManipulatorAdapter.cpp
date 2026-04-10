#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Utilities/StringAlgorithms.h>

ezSplineManipulatorAdapter::ezSplineManipulatorAdapter() = default;
ezSplineManipulatorAdapter::~ezSplineManipulatorAdapter() = default;

// static
ezResult ezSplineManipulatorAdapter::BuildSpline(const ezDocumentObject* pSplineComponent, ezStringView sClosedPropertyName, ezSpline& out_spline, ezStringView sNodeName /*= ezStringView()*/, ezUInt32* out_pNodeIndex /*= nullptr*/)
{
  out_spline.m_ControlPoints.Clear();
  out_spline.m_bClosed = false;

  if (pSplineComponent == nullptr)
    return EZ_FAILURE;

  const ezDocumentObject* pParent = pSplineComponent->GetParent();
  if (pParent == nullptr)
    return EZ_FAILURE;

  out_spline.m_bClosed = pSplineComponent->GetTypeAccessor().GetValue(sClosedPropertyName).ConvertTo<bool>();

  const ezIReflectedTypeAccessor& parentAccessor = pParent->GetTypeAccessor();
  const ezInt32 iChildCount = parentAccessor.GetCount("Children");
  for (ezInt32 i = 0; i < iChildCount; ++i)
  {
    ezVariant val = parentAccessor.GetValue("Children", i);
    if (!val.IsA<ezUuid>())
      continue;

    const ezDocumentObject* pChild = pParent->GetDocumentObjectManager()->GetObject(val.Get<ezUuid>());
    if (pChild == nullptr)
      continue;

    const ezDocumentObject* pNodeComponent = nullptr;
    for (const ezDocumentObject* pComp : pChild->GetChildren())
    {
      if (pComp->GetParentProperty() == "Components" && pComp->GetType()->GetTypeName() == "ezSplineNodeComponent")
      {
        pNodeComponent = pComp;
        break;
      }
    }

    if (pNodeComponent == nullptr)
      continue;

    ezSpline::ControlPoint cp;
    if (FillControlPointFromNodeComponent(pNodeComponent, cp).Succeeded())
    {
      out_spline.m_ControlPoints.PushBack(cp);

      if (out_pNodeIndex != nullptr && !sNodeName.IsEmpty())
      {
        ezVariant name = pChild->GetTypeAccessor().GetValue("Name");
        if (name.IsA<ezString>() && name.Get<ezString>() == sNodeName)
          *out_pNodeIndex = out_spline.m_ControlPoints.GetCount() - 1;
      }
    }
  }

  out_spline.CalculateUpDirAndAutoTangents();

  return EZ_SUCCESS;
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
  const ezSplineManipulatorAttribute* pAttr = static_cast<const ezSplineManipulatorAttribute*>(m_pManipulatorAttr);

  auto HasSplineProperties = [&](const ezDocumentObject* pObj) -> bool
  {
    const ezRTTI* pType = pObj->GetTypeAccessor().GetType();
    return pType->FindPropertyByName(pAttr->GetBindTo()) != nullptr &&
           pType->FindPropertyByName(pAttr->GetClosedProperty()) != nullptr;
  };

  // m_pObject may not directly expose the spline properties. Walk up the parent chain.
  // At each level, also check components (objects stored in the "Components" array property),
  // since the relevant object may be a component of a game object rather than the game object itself.
  while (m_pObject != nullptr)
  {
    if (HasSplineProperties(m_pObject))
      break;

    ezVariantArray componentUuids;
    if (m_pObject->GetTypeAccessor().GetValues("Components", componentUuids))
    {
      const ezDocumentObject* pFoundComponent = nullptr;
      for (const auto& v : componentUuids)
      {
        if (v.IsA<ezUuid>())
        {
          const ezDocumentObject* pComponent = m_pObject->GetDocumentObjectManager()->GetObject(v.Get<ezUuid>());
          if (pComponent != nullptr && HasSplineProperties(pComponent))
          {
            pFoundComponent = pComponent;
            break;
          }
        }
      }

      if (pFoundComponent != nullptr)
      {
        m_pObject = pFoundComponent;
        break;
      }
    }

    m_pObject = m_pObject->GetParent();
  }
}

void ezSplineManipulatorAdapter::Update()
{
  BuildSpline();
  ConfigureGizmos();
}

void ezSplineManipulatorAdapter::ClickGizmoEventHandler(const ezGizmoEvent& e)
{
  if (e.m_Type != ezGizmoEvent::Type::Interaction)
    return;

  e.m_pGizmo->GetOwnerView()->ClearLastPickedObject();

  ezInt32 index = -1;

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
  MakeUniqueName(index, sNewNodeName);

  index = ezMath::Min<ezInt32>(index, m_Spline.m_ControlPoints.GetCount());

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

  pObjectAcessor->FinishTransaction();

  Update();

  // Defer the selection change to avoid destroying this adapter (and its gizmos) while
  // their event dispatch is still on the call stack. SetSelection triggers ClearAdapters
  // via the manipulator manager, which deletes 'this'. The document outlives the adapter.
  const ezDocument* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  QTimer::singleShot(0, [pDoc, gameObjectUuid]()
    {
      if (auto pSelMan = pDoc->GetSelectionManager())
      {
        if (const ezDocumentObject* pObj = pDoc->GetObjectManager()->GetObject(gameObjectUuid))
        {
          pSelMan->SetSelection(pObj);
        }
      } });
}

/// Returns the position on the given spline segment at 50% arc-length.
/// Uses uniform sampling with a local parameter t in [0, 1] to avoid the
/// Bezier overshoot that occurs with EvaluatePosition(segmentIndex + 0.5f)
/// when adjacent segments have very different lengths.
static ezVec3 EvaluateSegmentArcLengthMidpoint(const ezSpline& spline, ezUInt32 uiSegment)
{
  constexpr ezUInt32 uiNumSamples = 16;

  float fCumulative[uiNumSamples + 1];
  fCumulative[0] = 0.0f;

  ezSimdVec4f vPrev = spline.EvaluatePosition(uiSegment, 0.0f);
  for (ezUInt32 k = 1; k <= uiNumSamples; ++k)
  {
    const float fLocalT = static_cast<float>(k) / uiNumSamples;
    const ezSimdVec4f vCur = spline.EvaluatePosition(uiSegment, fLocalT);
    fCumulative[k] = fCumulative[k - 1] + (vCur - vPrev).GetLength<3>();
    vPrev = vCur;
  }

  const float fHalfLength = fCumulative[uiNumSamples] * 0.5f;

  if (fHalfLength < ezMath::SmallEpsilon<float>())
    return ezSimdConversion::ToVec3(spline.EvaluatePosition(uiSegment, 0.5f));

  for (ezUInt32 k = 1; k <= uiNumSamples; ++k)
  {
    if (fCumulative[k] >= fHalfLength)
    {
      const float fFrac = ezMath::Unlerp(fCumulative[k - 1], fCumulative[k], fHalfLength);
      const float fLocalT = (static_cast<float>(k - 1) + fFrac) / uiNumSamples;
      return ezSimdConversion::ToVec3(spline.EvaluatePosition(uiSegment, fLocalT));
    }
  }

  return ezSimdConversion::ToVec3(spline.EvaluatePosition(uiSegment, 0.5f));
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

  const ezUInt32 uiNumCPs = m_Spline.m_ControlPoints.GetCount();

  ezUInt32 uiFirstGizmo = 0;
  ezUInt32 uiNumGizmos = m_Gizmos.GetCount();

  if (!m_Spline.m_bClosed)
  {
    if (uiNumCPs == 0)
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

    const ezVec3 offset = EvaluateSegmentArcLengthMidpoint(m_Spline, i);
    gizmo.SetTransformation(MakeGizmoTransform(offset));
  }
}

void ezSplineManipulatorAdapter::BuildSpline()
{
  m_Spline.m_ControlPoints.Clear();
  m_Spline.m_bClosed = false;

  const ezSplineManipulatorAttribute* pAttr = static_cast<const ezSplineManipulatorAttribute*>(m_pManipulatorAttr);
  if (pAttr->GetBindTo().IsEmpty() || pAttr->GetClosedProperty().IsEmpty())
    return;

  BuildSpline(m_pObject, pAttr->GetClosedProperty(), m_Spline).AssertSuccess();
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

void ezSplineManipulatorAdapter::MakeUniqueName(ezInt32 iIndex, ezStringBuilder& ref_sName)
{
  // Collect names of existing spline node children in array order.
  ezDynamicArray<ezString> nodeNames;
  const ezDocumentObject* pParent = m_pObject->GetParent();
  if (pParent != nullptr)
  {
    const ezIReflectedTypeAccessor& parentAccessor = pParent->GetTypeAccessor();
    const ezInt32 iChildCount = parentAccessor.GetCount("Children");
    for (ezInt32 i = 0; i < iChildCount; ++i)
    {
      ezVariant val = parentAccessor.GetValue("Children", i);
      if (!val.IsA<ezUuid>())
        continue;

      const ezDocumentObject* pChild = pParent->GetDocumentObjectManager()->GetObject(val.Get<ezUuid>());
      if (pChild == nullptr)
        continue;

      bool bHasSplineNode = false;
      for (const ezDocumentObject* pComp : pChild->GetChildren())
      {
        if (pComp->GetParentProperty() == "Components" && pComp->GetType()->GetTypeName() == "ezSplineNodeComponent")
        {
          bHasSplineNode = true;
          break;
        }
      }

      if (!bHasSplineNode)
        continue;

      ezVariant name = pChild->GetTypeAccessor().GetValue("Name");
      nodeNames.PushBack(name.IsA<ezString>() ? name.Get<ezString>() : ezString());
    }
  }

  if (nodeNames.IsEmpty())
  {
    ref_sName = "1";
    return;
  }

  auto IsUniqueName = [&](ezStringView sName) -> bool
  {
    for (const auto& s : nodeNames)
    {
      if (s == sName)
        return false;
    }
    return true;
  };

  const ezStringView sLeft = (iIndex > 0 && iIndex <= (ezInt32)nodeNames.GetCount()) ? nodeNames[iIndex - 1].GetView() : ezStringView();
  const ezStringView sRight = (iIndex < (ezInt32)nodeNames.GetCount()) ? nodeNames[iIndex].GetView() : ezStringView();

  ezStringAlgorithms::ComputeNameBetween(sLeft, sRight, ref_sName);

  if (IsUniqueName(ref_sName))
    return;

  // Fallback: append an incrementing sub-index until the name is unique
  ezStringBuilder sBase = ref_sName;
  ezUInt32 uiSuffix = 1;
  do
  {
    ref_sName = sBase;
    ref_sName.AppendFormat(".{}", uiSuffix);
    ++uiSuffix;
  } while (!IsUniqueName(ref_sName));
}
