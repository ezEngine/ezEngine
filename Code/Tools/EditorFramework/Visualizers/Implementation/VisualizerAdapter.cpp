#include <EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezVisualizerAdapter::ezVisualizerAdapter()
{
  m_pVisualizerAttr = nullptr;
  m_pObject = nullptr;
  m_bVisualizerIsVisible = true;

  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentWindowEventHandler, this));
}

ezVisualizerAdapter::~ezVisualizerAdapter()
{
  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(
        ezMakeDelegate(&ezVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(
        ezMakeDelegate(&ezVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void ezVisualizerAdapter::SetVisualizer(const ezVisualizerAttribute* pAttribute, const ezDocumentObject* pObject)
{
  m_pVisualizerAttr = pAttribute;
  m_pObject = pObject;

  auto& meta = m_pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bVisualizerIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}


ezQuat ezVisualizerAdapter::GetBasisRotation(ezBasisAxis::Enum identity, ezBasisAxis::Enum axis)
{
  ezQuat rotId;
  switch (identity)
  {
    case ezBasisAxis::PositiveX:
      rotId.SetIdentity();
      break;
    case ezBasisAxis::PositiveY:
      rotId.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::PositiveZ:
      rotId.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      break;
    case ezBasisAxis::NegativeX:
      rotId.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
      break;
    case ezBasisAxis::NegativeY:
      rotId.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      break;
    case ezBasisAxis::NegativeZ:
      rotId.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      break;
  }

    ezQuat rotAxis;
  switch (axis)
  {
    case ezBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case ezBasisAxis::PositiveY:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      break;
    case ezBasisAxis::PositiveZ:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeX:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
      break;
    case ezBasisAxis::NegativeY:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeZ:
      rotAxis.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
      break;
  }

  return rotId * rotAxis;
}

void ezVisualizerAdapter::DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pVisualizerAttr->m_sProperty1 || e.m_sProperty == m_pVisualizerAttr->m_sProperty2 ||
          e.m_sProperty == m_pVisualizerAttr->m_sProperty3 || e.m_sProperty == m_pVisualizerAttr->m_sProperty4 ||
          e.m_sProperty == m_pVisualizerAttr->m_sProperty5)
      {
        Update();
      }
    }
  }
}

void ezVisualizerAdapter::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  if (e.m_Type == ezQtDocumentWindowEvent::BeforeRedraw &&
      e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument())
  {
    UpdateGizmoTransform();
  }
}

void ezVisualizerAdapter::DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & ezDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bVisualizerIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

ezTransform ezVisualizerAdapter::GetObjectTransform() const
{
  ezTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t);

  return t;
}

ezObjectAccessorBase* ezVisualizerAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const ezAbstractProperty* ezVisualizerAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}
