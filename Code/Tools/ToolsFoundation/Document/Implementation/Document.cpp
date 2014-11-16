#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentBase, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEvent<const ezDocumentBase::Event&> ezDocumentBase::s_EventsAny;

ezDocumentBase::ezDocumentBase(const char* szPath) :
  m_ObjectTree(this)
{
  m_sDocumentPath = szPath;
  m_pCommandHistory = nullptr;
  m_pObjectManager = nullptr;
  m_SelectionManager.SetOwner(this);

  m_bModified = false;
  m_bReadOnly = false;
}

ezDocumentBase::~ezDocumentBase()
{
  m_SelectionManager.SetOwner(nullptr);
}

void ezDocumentBase::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void ezDocumentBase::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocumentBase::SaveDocument()
{
  ezStatus ret = InternalSaveDocument();

  if (ret.m_Result.Succeeded())
  {
    Event e;
    e.m_pDocument = this;
    e.m_Type = Event::Type::DocumentSaved;
    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);

    SetModified(false);
  }

  return ret;
}

void ezDocumentBase::EnsureVisible()
{
  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

