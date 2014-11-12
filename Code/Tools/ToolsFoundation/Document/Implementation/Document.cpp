#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentBase, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

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

  m_Events.Broadcast(e);
}

void ezDocumentBase::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ReadOnlyChanged;

  m_Events.Broadcast(e);
}

ezStatus ezDocumentBase::SaveDocument()
{
  ezStatus ret = InternalSaveDocument();

  if (ret.m_Result.Succeeded())
    SetModified(false);

  return ret;
}