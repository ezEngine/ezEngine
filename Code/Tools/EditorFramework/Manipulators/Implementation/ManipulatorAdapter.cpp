#include <EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezManipulatorAdapter::ezManipulatorAdapter()
{
  m_pManipulatorAttr = nullptr;
  m_pObject = nullptr;
  m_bManipulatorIsVisible = true;

  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentWindowEventHandler, this));
}

ezManipulatorAdapter::~ezManipulatorAdapter()
{
  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(
        ezMakeDelegate(&ezManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(
        ezMakeDelegate(&ezManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void ezManipulatorAdapter::SetManipulator(const ezManipulatorAttribute* pAttribute, const ezDocumentObject* pObject)
{
  m_pManipulatorAttr = pAttribute;
  m_pObject = pObject;

  auto& meta = m_pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bManipulatorIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}

void ezManipulatorAdapter::DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pManipulatorAttr->m_sProperty1 || e.m_sProperty == m_pManipulatorAttr->m_sProperty2 ||
          e.m_sProperty == m_pManipulatorAttr->m_sProperty3 || e.m_sProperty == m_pManipulatorAttr->m_sProperty4 ||
          e.m_sProperty == m_pManipulatorAttr->m_sProperty5 || e.m_sProperty == m_pManipulatorAttr->m_sProperty6)
      {
        Update();
      }
    }
  }
}

void ezManipulatorAdapter::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  if (e.m_Type == ezQtDocumentWindowEvent::BeforeRedraw &&
      e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument())
  {
    UpdateGizmoTransform();
  }
}

void ezManipulatorAdapter::DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & ezDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bManipulatorIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

ezTransform ezManipulatorAdapter::GetObjectTransform() const
{
  ezTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t);

  return t;
}

ezObjectAccessorBase* ezManipulatorAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const ezAbstractProperty* ezManipulatorAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}

void ezManipulatorAdapter::BeginTemporaryInteraction()
{
  GetObjectAccessor()->BeginTemporaryCommands("Adjust Object");
}

void ezManipulatorAdapter::EndTemporaryInteraction()
{
  GetObjectAccessor()->FinishTemporaryCommands();
}

void ezManipulatorAdapter::CancelTemporayInteraction()
{
  GetObjectAccessor()->CancelTemporaryCommands();
}

void ezManipulatorAdapter::ClampProperty(const char* szProperty, ezVariant& value) const
{
  ezResult status(EZ_FAILURE);
  const double fCur = value.ConvertTo<double>(&status);

  if (status.Failed())
    return;

  const ezClampValueAttribute* pClamp = GetProperty(szProperty)->GetAttributeByType<ezClampValueAttribute>();
  if (pClamp == nullptr)
    return;

  if (pClamp->GetMinValue().IsValid())
  {
    const double fMin = pClamp->GetMinValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur < fMin)
        value = pClamp->GetMinValue();
    }
  }

  if (pClamp->GetMaxValue().IsValid())
  {
    const double fMax = pClamp->GetMaxValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur > fMax)
        value = pClamp->GetMaxValue();
    }
  }
}

void ezManipulatorAdapter::ChangeProperties(const char* szProperty1, ezVariant value1, const char* szProperty2 /*= nullptr*/,
                                            ezVariant value2 /*= ezVariant()*/, const char* szProperty3 /*= nullptr*/,
                                            ezVariant value3 /*= ezVariant()*/, const char* szProperty4 /*= nullptr*/,
                                            ezVariant value4 /*= ezVariant()*/, const char* szProperty5 /*= nullptr*/,
                                            ezVariant value5 /*= ezVariant()*/, const char* szProperty6 /*= nullptr*/,
                                            ezVariant value6 /*= ezVariant()*/)
{
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  pObjectAccessor->StartTransaction("Change Properties");

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = m_pObject->GetGuid();

  if (!ezStringUtils::IsNullOrEmpty(szProperty1))
  {
    ClampProperty(szProperty1, value1);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty1), value1);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty2))
  {
    ClampProperty(szProperty2, value2);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty2), value2);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty3))
  {
    ClampProperty(szProperty3, value3);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty3), value3);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty4))
  {
    ClampProperty(szProperty4, value4);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty4), value4);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty5))
  {
    ClampProperty(szProperty5, value5);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty5), value5);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty6))
  {
    ClampProperty(szProperty6, value6);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty6), value6);
  }

  pObjectAccessor->FinishTransaction();
}
