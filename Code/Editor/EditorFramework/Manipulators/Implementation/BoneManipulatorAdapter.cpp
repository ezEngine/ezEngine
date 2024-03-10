#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezString ezBoneManipulatorAdapter::s_sLastSelectedBone;

ezBoneManipulatorAdapter::ezBoneManipulatorAdapter() = default;
ezBoneManipulatorAdapter::~ezBoneManipulatorAdapter() = default;

void ezBoneManipulatorAdapter::Finalize()
{
  RetrieveBones();
  ConfigureGizmos();
  MigrateSelection();
}

void ezBoneManipulatorAdapter::MigrateSelection()
{
  for (ezUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (m_Bones[i].m_sName == s_sLastSelectedBone)
    {
      m_Gizmos[i].m_RotateGizmo.SetVisible(true);
      m_Gizmos[i].m_ClickGizmo.SetVisible(false);
      return;
    }
  }

  // keep the last selection, even if it can't be migrated, until something else gets selected
}

void ezBoneManipulatorAdapter::Update()
{
  RetrieveBones();
  UpdateGizmoTransform();
}

void ezBoneManipulatorAdapter::RotateGizmoEventHandler(const ezGizmoEvent& e)
{
  ezUInt32 uiGizmo = ezInvalidIndex;

  for (ezUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_RotateGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      break;
    }
  }

  EZ_ASSERT_DEBUG(uiGizmo != ezInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == ezInvalidIndex)
    return;

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
      ezTransform globalGizmo = static_cast<const ezGizmo*>(e.m_pGizmo)->GetTransformation();
      globalGizmo.m_vScale.Set(1);

      ezMat4 mGizmo = globalGizmo.GetAsMat4();
      mGizmo = GetObjectTransform().GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_RootTransform.GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_Gizmos[uiGizmo].m_InverseOffset * mGizmo;

      ezQuat rotOnly;
      rotOnly.ReconstructFromMat4(mGizmo);

      SetTransform(uiGizmo, ezTransform(mGizmo.GetTranslationVector(), rotOnly));
    }
    break;
  }
}

void ezBoneManipulatorAdapter::ClickGizmoEventHandler(const ezGizmoEvent& e)
{
  ezUInt32 uiGizmo = ezInvalidIndex;
  s_sLastSelectedBone.Clear();

  for (ezUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_ClickGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      s_sLastSelectedBone = m_Bones[gIdx].m_sName;
      break;
    }
  }

  EZ_ASSERT_DEBUG(uiGizmo != ezInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == ezInvalidIndex)
    return;

  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::Interaction:
    {
      for (ezUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
      {
        m_Gizmos[i].m_RotateGizmo.SetVisible(false);
        m_Gizmos[i].m_ClickGizmo.SetVisible(true);
      }

      m_Gizmos[uiGizmo].m_RotateGizmo.SetVisible(true);
      m_Gizmos[uiGizmo].m_ClickGizmo.SetVisible(false);
    }
    break;
    default:
      break;
  }
}

void ezBoneManipulatorAdapter::RetrieveBones()
{
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const ezBoneManipulatorAttribute* pAttr = static_cast<const ezBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  ezVariantArray values;

  // Exposed parameters are only stored as diffs in the component. Thus, requesting the exposed parameters only returns those that have been modified. To get all, you need to use the ezExposedParameterCommandAccessor which gives you all exposed parameters from the source asset.
  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  if (const ezExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<ezExposedParametersAttribute>())
  {
    const ezAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
    EZ_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

    ezExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);
    proxy.GetValues(m_pObject, pProperty, values).AssertSuccess();
    proxy.GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
  }
  else
  {
    pObjectAccessor->GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
    pObjectAccessor->GetValues(m_pObject, pProperty, values).AssertSuccess();
  }

  m_RootTransform.SetIdentity();

  m_Bones.Clear();
  m_Bones.SetCount(values.GetCount());

  for (ezUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (values[i].GetReflectedType() == ezGetStaticRTTI<ezExposedBone>())
    {
      const ezExposedBone* pBone = reinterpret_cast<const ezExposedBone*>(values[i].GetData());

      if (pBone->m_sName == "<root-transform>")
      {
        m_RootTransform = pBone->m_Transform;
      }

      m_Bones[i] = *pBone;
    }
    else
    {
      // EZ_REPORT_FAILURE("Property is not an ezExposedBone");
      m_Bones.Clear();
      return;
    }
  }
}

void ezBoneManipulatorAdapter::UpdateGizmoTransform()
{
  const ezMat4 ownerTransform = GetObjectTransform().GetAsMat4();

  for (ezUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);
    gizmo.m_InverseOffset = gizmo.m_Offset.GetInverse();

    ezMat4 mGizmo = ownerTransform * m_RootTransform.GetAsMat4() * gizmo.m_Offset * m_Bones[i].m_Transform.GetAsMat4();

    ezQuat rotOnly;
    rotOnly.ReconstructFromMat4(mGizmo);

    ezTransform tGizmo;
    tGizmo.m_vPosition = mGizmo.GetTranslationVector();
    tGizmo.m_qRotation = rotOnly;

    tGizmo.m_vScale.Set(0.5f);
    gizmo.m_RotateGizmo.SetTransformation(tGizmo);

    tGizmo.m_vScale.Set(0.02f);
    gizmo.m_ClickGizmo.SetTransformation(tGizmo);
  }
}

void ezBoneManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);
  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmos.SetCount(m_Bones.GetCount());

  for (ezUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);

    auto& rot = gizmo.m_RotateGizmo;
    rot.SetOwner(pEngineWindow, nullptr);
    rot.SetVisible(false);
    rot.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezBoneManipulatorAdapter::RotateGizmoEventHandler, this));

    auto& click = gizmo.m_ClickGizmo;
    click.SetOwner(pEngineWindow, nullptr);
    click.SetVisible(true);
    click.SetColor(ezColor::Thistle);
    click.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezBoneManipulatorAdapter::ClickGizmoEventHandler, this));
  }

  UpdateGizmoTransform();
}

void ezBoneManipulatorAdapter::SetTransform(ezUInt32 uiBone, const ezTransform& value)
{
  ezExposedBone* pBone = &m_Bones[uiBone];

  pBone->m_Transform.m_qRotation = value.m_qRotation;
  pBone->m_Transform = value;

  ezExposedBone bone;
  bone.m_sName = pBone->m_sName;
  bone.m_sParent = pBone->m_sParent;
  bone.m_Transform = value;

  ezVariant var;
  var.CopyTypedObject(&bone, ezGetStaticRTTI<ezExposedBone>());

  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const ezBoneManipulatorAttribute* pAttr = static_cast<const ezBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  const ezExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<ezExposedParametersAttribute>();

  const ezAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
  EZ_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

  ezExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);

  // for some reason the first command in ezExposedParameterCommandAccessor returns failure 'the property X does not exist' and the insert
  // command than fails with 'the property X already exists' ???

  proxy.SetValue(m_pObject, pProperty, var, m_Keys[uiBone]).AssertSuccess();
}

ezMat4 ezBoneManipulatorAdapter::ComputeFullTransform(ezUInt32 uiBone) const
{
  const ezMat4 tParent = ComputeParentTransform(uiBone);

  return tParent * m_Bones[uiBone].m_Transform.GetAsMat4();
}

ezMat4 ezBoneManipulatorAdapter::ComputeParentTransform(ezUInt32 uiBone) const
{
  const ezString& parent = m_Bones[uiBone].m_sParent;

  if (!parent.IsEmpty())
  {
    for (ezUInt32 b = 0; b < m_Bones.GetCount(); ++b)
    {
      if (m_Bones[b].m_sName == parent)
      {
        return ComputeFullTransform(b);
      }
    }
  }

  return ezMat4::MakeIdentity();
}
