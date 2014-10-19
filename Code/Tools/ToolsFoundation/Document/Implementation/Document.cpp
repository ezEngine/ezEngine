#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>

ezDocumentBase::ezDocumentBase() :
  m_ObjectTree(this)
{
  m_pCommandHistory = nullptr;
  m_pObjectManager = nullptr;
  m_SelectionManager.SetOwner(this);
}

ezDocumentBase::~ezDocumentBase()
{
  m_SelectionManager.SetOwner(nullptr);
}

void ezDocumentBase::SetModified(bool b)
{
  m_bModified = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ModifiedChanged;

  m_Events.Broadcast(e);
}

void ezDocumentBase::SetReadOnly(bool b)
{
  m_bReadOnly = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ReadOnlyChanged;

  m_Events.Broadcast(e);
}
