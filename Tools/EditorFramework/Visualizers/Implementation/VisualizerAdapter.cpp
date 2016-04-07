#include <PCH.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Math/Transform.h>

ezVisualizerAdapter::ezVisualizerAdapter()
{
  m_pVisualizerAttr = nullptr;
  m_pObject = nullptr;

  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentWindowEventHandler, this));
}

ezVisualizerAdapter::~ezVisualizerAdapter()
{
  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
  }
}

void ezVisualizerAdapter::SetVisualizer(const ezVisualizerAttribute* pAttribute, const ezDocumentObject* pObject)
{
  m_pVisualizerAttr = pAttribute;
  m_pObject = pObject;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezVisualizerAdapter::DocumentObjectPropertyEventHandler, this));

  Finalize();

  Update();
}

void ezVisualizerAdapter::DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sPropertyPath == m_pVisualizerAttr->m_sProperty1 ||
          e.m_sPropertyPath == m_pVisualizerAttr->m_sProperty2 ||
          e.m_sPropertyPath == m_pVisualizerAttr->m_sProperty3 ||
          e.m_sPropertyPath == m_pVisualizerAttr->m_sProperty4)
      {
        Update();
      }
    }
  }
}

void ezVisualizerAdapter::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  if (e.m_Type == ezQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument())
  {
    UpdateGizmoTransform();
  }
}

ezTransform ezVisualizerAdapter::GetObjectTransform() const
{
  ezTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t);

  return t;
}




