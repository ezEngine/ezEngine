#include <PCH.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Core/World/GameObject.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

ezManipulatorAdapter::ezManipulatorAdapter()
{
  m_pManipulatorAttr = nullptr;
  m_pObject = nullptr;

  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentWindowEventHandler, this));
}

ezManipulatorAdapter::~ezManipulatorAdapter()
{
  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
  }
}

void ezManipulatorAdapter::SetManipulator(const ezManipulatorAttribute* pAttribute, const ezDocumentObject* pObject)
{
  m_pManipulatorAttr = pAttribute;
  m_pObject = pObject;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezManipulatorAdapter::DocumentObjectPropertyEventHandler, this));

  Finalize();

  Update();
}

void ezManipulatorAdapter::DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sPropertyPath == m_pManipulatorAttr->m_sProperty1 ||
          e.m_sPropertyPath == m_pManipulatorAttr->m_sProperty2 ||
          e.m_sPropertyPath == m_pManipulatorAttr->m_sProperty3 ||
          e.m_sPropertyPath == m_pManipulatorAttr->m_sProperty4)
      {
        Update();
      }
    }
  }
}

void ezManipulatorAdapter::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  if (e.m_Type == ezQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument())
  {
    UpdateGizmoTransform();
  }
}

ezTransform ezManipulatorAdapter::GetObjectTransform() const
{
  ezTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t);

  return t;
}

void ezManipulatorAdapter::BeginTemporaryInteraction()
{
  m_pObject->GetDocumentObjectManager()->GetDocument()->GetCommandHistory()->BeginTemporaryCommands();
}

void ezManipulatorAdapter::EndTemporaryInteraction()
{
  m_pObject->GetDocumentObjectManager()->GetDocument()->GetCommandHistory()->FinishTemporaryCommands();
}

void ezManipulatorAdapter::CancelTemporayInteraction()
{
  m_pObject->GetDocumentObjectManager()->GetDocument()->GetCommandHistory()->CancelTemporaryCommands();
}

void ezManipulatorAdapter::ChangeProperties(const char* szProperty1, const ezVariant& value1, const char* szProperty2 /*= nullptr*/, const ezVariant& value2 /*= ezVariant()*/, const char* szProperty3 /*= nullptr*/, const ezVariant& value3 /*= ezVariant()*/, const char* szProperty4 /*= nullptr*/, const ezVariant& value4 /*= ezVariant()*/)
{
  auto pHistory = m_pObject->GetDocumentObjectManager()->GetDocument()->GetCommandHistory();
  pHistory->StartTransaction();

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = m_pObject->GetGuid();

  if (!ezStringUtils::IsNullOrEmpty(szProperty1))
  {
    cmd.SetPropertyPath(szProperty1);
    cmd.m_NewValue = value1;
    pHistory->AddCommand(cmd);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty2))
  {
    cmd.SetPropertyPath(szProperty2);
    cmd.m_NewValue = value2;
    pHistory->AddCommand(cmd);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty3))
  {
    cmd.SetPropertyPath(szProperty3);
    cmd.m_NewValue = value3;
    pHistory->AddCommand(cmd);
  }

  if (!ezStringUtils::IsNullOrEmpty(szProperty4))
  {
    cmd.SetPropertyPath(szProperty4);
    cmd.m_NewValue = value4;
    pHistory->AddCommand(cmd);
  }

  pHistory->FinishTransaction();
}



