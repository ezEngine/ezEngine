#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxManipulatorAdapter::ezBoxManipulatorAdapter() = default;
ezBoxManipulatorAdapter::~ezBoxManipulatorAdapter() = default;

void ezBoxManipulatorAdapter::QueryGridSettings(ezGridSettingsMsgToEngine& out_gridSettings)
{
  out_gridSettings.m_vGridCenter = m_Gizmo.GetTransformation().m_vPosition;

  // if density != 0, it is enabled at least in ortho mode
  out_gridSettings.m_fGridDensity = ezSnapProvider::GetTranslationSnapValue();

  // to be active in perspective mode, tangents have to be non-zero
  out_gridSettings.m_vGridTangent1.SetZero();
  out_gridSettings.m_vGridTangent2.SetZero();
}

void ezBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezBoxManipulatorAdapter::GizmoEventHandler, this));
}

void ezBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezBoxManipulatorAttribute* pAttr = static_cast<const ezBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    ezVec3 vSize = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
    vSize *= pAttr->m_fSizeScale;
    vSize *= 0.5f;

    m_vOldSize = vSize;

    m_Gizmo.SetSize(vSize, vSize, false);
  }

  m_vPositionOffset.SetZero();

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    m_vPositionOffset = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetOffsetProperty()));
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<ezQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  UpdateGizmoTransform();
}

void ezBoxManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
{
  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
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
      const ezBoxManipulatorAttribute* pAttr = static_cast<const ezBoxManipulatorAttribute*>(m_pManipulatorAttr);

      const char* szSizeProperty = pAttr->GetSizeProperty();
      const ezVec3 vNewSizeNeg = m_Gizmo.GetNegSize();
      const ezVec3 vNewSizePos = m_Gizmo.GetPosSize();
      const ezVec3 vNewSize = (vNewSizeNeg + vNewSizePos) / pAttr->m_fSizeScale;

      ezVariant oldSize;

      ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

      pObjectAccessor->GetValue(m_pObject, GetProperty(szSizeProperty), oldSize);

      const ezVec3 vOldSize = oldSize.ConvertTo<ezVec3>();

      ezVariant newValue = vNewSize;

      pObjectAccessor->StartTransaction("Change Properties");

      if (!ezStringUtils::IsNullOrEmpty(szSizeProperty))
      {
        pObjectAccessor->SetValue(m_pObject, GetProperty(szSizeProperty), newValue);
      }

      if (pAttr->m_bRecenterParent)
      {
        const ezDocumentObject* pParent = m_pObject->GetParent();

        if (const ezGameObjectDocument* pGameDoc = ezDynamicCast<const ezGameObjectDocument*>(pParent->GetDocumentObjectManager()->GetDocument()))
        {
          ezTransform tParent = pGameDoc->GetGlobalTransform(pParent);

          ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

          if (m_vOldSize.x != vNewSizeNeg.x)
            tParent.m_vPosition -= tParent.m_qRotation * ezVec3((vNewSizeNeg.x - m_vOldSize.x) * 0.5f, 0, 0);
          if (m_vOldSize.x != vNewSizePos.x)
            tParent.m_vPosition += tParent.m_qRotation * ezVec3((vNewSizePos.x - m_vOldSize.x) * 0.5f, 0, 0);

          if (m_vOldSize.y != vNewSizeNeg.y)
            tParent.m_vPosition -= tParent.m_qRotation * ezVec3(0, (vNewSizeNeg.y - m_vOldSize.y) * 0.5f, 0);
          if (m_vOldSize.y != vNewSizePos.y)
            tParent.m_vPosition += tParent.m_qRotation * ezVec3(0, (vNewSizePos.y - m_vOldSize.y) * 0.5f, 0);

          if (m_vOldSize.z != vNewSizeNeg.z)
            tParent.m_vPosition -= tParent.m_qRotation * ezVec3(0, 0, (vNewSizeNeg.z - m_vOldSize.z) * 0.5f);
          if (m_vOldSize.z != vNewSizePos.z)
            tParent.m_vPosition += tParent.m_qRotation * ezVec3(0, 0, (vNewSizePos.z - m_vOldSize.z) * 0.5f);

          pGameDoc->SetGlobalTransform(pParent, tParent, TransformationChanges::Translation);
        }
      }

      pObjectAccessor->FinishTransaction();
    }

    break;
  }
}

void ezBoxManipulatorAdapter::UpdateGizmoTransform()
{
  ezTransform t;
  t.m_vScale.Set(1);
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_Gizmo.SetTransformation(GetObjectTransform() * t);
}
