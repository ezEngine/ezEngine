#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetEditorTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetEditorTypeAccessor());
}

ezUInt32 ezDocumentObjectBase::GetChildIndex(ezDocumentObjectBase* pChild) const
{
  return m_Children.IndexOf(pChild);
}

