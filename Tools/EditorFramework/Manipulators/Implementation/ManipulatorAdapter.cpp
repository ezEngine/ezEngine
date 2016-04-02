#include <PCH.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezManipulatorAdapter::ezManipulatorAdapter()
{
  m_pManipulatorAttr = nullptr;
  m_pObject = nullptr;
}

ezManipulatorAdapter::~ezManipulatorAdapter()
{
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
